#include <iostream>
#include <chrono>
#include <cmath>
#include <vector>
#include <bits/stdc++.h>
#include <unordered_map>
#include <list>
#include <algorithm>
#include <utility>
#include "SDL2/SDL.h"
#include "tables.h"
#include "position.h"
#include "types.h"
#include "position.cpp"
#include "tables.cpp"
#include "types.cpp"
#include "evaluate.h"

using namespace std;

unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
PRNG rng(seed);

const float ExplorationConstant = 5000.0;

unordered_map<uint64_t, float> tacticalTransposition;



int debugcounter = 0;


struct Moveval{
	Move move;
	float val;

	Moveval (Move x, float y): move(x), val(y) {}

	bool operator<(const Moveval &m) const {	
		return val < m.val;
	}

	bool operator>(const Moveval &m) const {	
		return val > m.val;
	}

};

template <Color Us>
vector<Move> generateCaptures(Position p){
    vector<Move> captureList;
    MoveList<Us> mlist(p);
    for(Move move : mlist){
		if (move.is_capture()){
            captureList.push_back(move);
        }
		else{ 
			p.play<Us>(move);
			if(p.in_check<~Us>()){
				captureList.push_back(move);
			}
			p.undo<Us>(move);
		}
        
    }
    return(captureList);
}

vector<Move> orderMoves(vector<Move> moveList, Position p){
	vector<Moveval> moveValues;
	for (Move move : moveList){
		Moveval m = Moveval(move, moveValue(move, p));
		moveValues.push_back(m);
	}
	std::sort(moveValues.begin(), moveValues.end(), std::greater<Moveval>());
	vector<Move> result;
	for(Moveval m : moveValues) {
		result.push_back(m.move);
	}
	return result;

}

// void BinaryInsert(vector<Node*> nodelist, Node* insertnode, int start = 0, int end = 0) {
// 	end = nodelist.size();
// 	int checklow = floor((end-start)/2)+start;
// 	int checkhigh = checklow+1;
// 	if (insertnode->eval == nodelist[check]->eval){
// 		nodelist.insert(nodelist.begin()+check, insertnode);
// 		return;
// 	}
// 	else if (insertnode > nodelist[check]) {
// 		BinaryInsert(nodelist, insertnode, start, check);
// 	}
// 	else {
// 		BinaryInsert(nodelist, insertnode, check, end);
// 	}
// }

class Node{
	public:
		bool root;
		bool leaf;
		unsigned long long timesSearched;
		float eval;
		vector<Node*> children;
		Node* parent;
		Position position;
		Move move;
		Color Us;
		
		struct PointerCompare {
			bool operator()(const Node* l, const Node* r) {
				return l->eval > r->eval;
      		}
    	};

		Node() {
			root = false;
			leaf = true;
			timesSearched = 0;
			eval = 0;
		}

		

		void backPropagate(int times = 1, bool carrying = true, int depth = 0){
			if (carrying) {
				sort(children.begin(), children.end(), PointerCompare());
				if (children[0]->eval == -eval){
					carrying = false;
				}
				else{
					eval = -1 * children[0]->eval;
				}
			}
			timesSearched += times;

			if (root){
				return;
			}
			depth++;
			if (depth > debugcounter){
				debugcounter = depth;
			}
			
			(*parent).backPropagate(times, carrying, depth);
		}
		template <Color Us>
		void expand(unordered_map<uint64_t, Node*> *NodeTranspositionTable){
			leaf = false;
			MoveList<Us> list(position);
			if(list.size() == 0 || children.size() > 0){
				eval = evaluate<Us>(position);
				if (!root){
					(*parent).backPropagate(1);
				}
				return;
			}
			float maxeval = -INFINITY;
			for (Move move : list){
				Node* n = new Node; //delete when a node is selected to be played
				n->parent = this;
				n->move = move;
				n->position = position;
				n->Us = ~(this->Us);
				n->position.play<Us>(move);
				if (NodeTranspositionTable->find(n->position.get_hash()) != NodeTranspositionTable->end()){
					children.push_back((*NodeTranspositionTable)[n->position.get_hash()]); //maybe just set n equal to the transposed node
					n->eval = (*NodeTranspositionTable)[n->position.get_hash()]->eval;
					n->timesSearched = (*NodeTranspositionTable)[n->position.get_hash()]->timesSearched;
					timesSearched += n->timesSearched;
				}
				else{
					(*NodeTranspositionTable)[n->position.get_hash()] = n;
					n->timesSearched = 1;
					timesSearched++;
					n->eval = -n->tactical<~Us>(); 
					children.push_back(n);
				}
				
			}
			
			sort(children.begin(), children.end(), PointerCompare());
			eval = -children[0]->eval;
			if (!root){
				(*parent).backPropagate(timesSearched);
			}
		}
		
		Node* select(int ParentNodes, unordered_map<uint64_t, Node*> *alreadySeen = new unordered_map<uint64_t, Node*>){ //using a cycle detector might be necessary as simply ignoring nodes we have already selected might cause problems

			if(leaf || children.size() == 0){
				delete alreadySeen;
				return(this);
			}
			float max = -INFINITY;
			Node* maxNode;
			for(Node* child : children){
				if (alreadySeen->find(child->position.get_hash()) != alreadySeen->end()){
					continue;
				}
				if (child->timesSearched == 0){
					delete alreadySeen;
					return(child);
				}
				float val = child->eval + sqrt((ExplorationConstant*log(ParentNodes))/(float)child->timesSearched);
				if (val > max){
					max = val;
					maxNode = child;
				}
			}
			if (max == -INFINITY){
				delete alreadySeen;
				return this;
			}
			(*alreadySeen)[maxNode->position.get_hash()] = maxNode;
			return maxNode->select(this->timesSearched, alreadySeen);
		}
		
		template <Color Us>
		float tactical(float alpha = -INFINITY, float beta = INFINITY){
			if(tacticalTransposition.find(position.get_hash()) != tacticalTransposition.end()){
				return tacticalTransposition[position.get_hash()];
			}
			float e = evaluate<Us>(position);
			if (e >= beta){
				tacticalTransposition[position.get_hash()] = beta;
				return beta;
			}
			if (e > alpha) {
				alpha = e;
			}
			vector<Move> mlist = generateCaptures<Us>(position);
			mlist = orderMoves(mlist, position);
			for(Move move : mlist){
				position.play<Us>(move);
				e = -tactical<~Us>(-beta, -alpha);
				position.undo<Us>(move);
				
				if (e >= beta){
					tacticalTransposition[position.get_hash()] = beta;
					return beta;
				}
				if (e > alpha) {
					alpha = e;
				}
				
			} 
			tacticalTransposition[position.get_hash()] = alpha;
			return alpha;
		}

		
};

unordered_map<uint64_t, Node*> NodeTranspositionTable;

template<Color Us>
unsigned long long perft(Position& p, unsigned int depth) {
	unsigned long long nodes = 0;

	MoveList<Us> list(p);

	if (depth == 1) return (unsigned long long) list.size();

	for (Move move : list) {
		p.play<Us>(move);
		nodes += perft<~Us>(p, depth - 1);
		p.undo<Us>(move);
	}

	return nodes;
}


template<Color Us>
void perftdiv(Position& p, unsigned int depth) {
	unsigned long long nodes = 0, pf;

	MoveList<Us> list(p);

	for (Move move : list) {
		std::cout << move;

		p.play<Us>(move);
		pf = perft<~Us>(p, depth - 1);
		std::cout << ": " << pf << " moves\n";
		nodes += pf;
		p.undo<Us>(move);
	}

	std::cout << "\nTotal: " << nodes << " moves\n";
}

void test_perft() {
	Position p;
	Position::set("rnbqkbnr/pppppppp/8/8/8/8/PPPP1PPP/RNBQKBNR w KQkq -", p);
	std::cout << p;

	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	auto n = perft<WHITE>(p, 6);
	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	auto diff = end - begin;

	std::cout << "Nodes: " << n << "\n";
	std::cout << "NPS: "
		<< int(n * 1000000.0 / std::chrono::duration_cast<std::chrono::microseconds>(diff).count())
		<< "\n";
	std::cout << "Time difference = "
		<< std::chrono::duration_cast<std::chrono::microseconds>(diff).count() << " [microseconds]\n";
}



void Tree(){
	Node root;
	root.root = true;
	root.leaf = false;
	
	string fen;
	getline(cin, fen);
	Position::set(fen, root.position);
	root.Us = root.position.side_to_play;
	if (root.Us== WHITE){
			root.template expand<WHITE>(&NodeTranspositionTable);
	}
	else{
		root.template expand<BLACK>(&NodeTranspositionTable);
	}
	bool Break = false;
	if (root.children.size() == 1){
		Break = true;
	}

	chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

	while (std::chrono::steady_clock::now() - start < std::chrono::seconds(5))
	{
		if (Break){
			break;
		}
		Node* selected = root.select(root.timesSearched);
		if (selected->Us == BLACK){
			selected->template expand<BLACK>(&NodeTranspositionTable);
		}
		else {
			selected->template expand<WHITE>(&NodeTranspositionTable);
		}
		
	}
	


	Node* bestmove;
	float max = -INFINITY;
	for(Node* child : root.children){
		if (child->eval > max){
			max = child->eval;
			bestmove = child;
			
		}
	}

	if (bestmove->move.move == 8455){ //short castling prints the wrong move
		cout << "e1g1";
	}
	else if (bestmove->move.move == 12095){
		cout << "e8g8";
	}
	else{
		cout << bestmove->move;
	}

}

int main(int argc, char *argv[]) {
	//Make sure to initialise all databases before using the library!
	initialise_all_databases();
	zobrist::initialise_zobrist_keys();
	
	Position p;
    Position::set("8/4k3/4r3/8/8/4q3/4R3/3K4 w - - 0 1", p);
	
	Node testnode;
	testnode.position = p;
	testnode.Us = WHITE;
	
	//test_perft();

   	Tree(); // do tomorrow: store the best tactical line, then search only quiet moves when the node is reached. or store tacitcal moves in transposition table.
	// crashes on rnbr4/1pp3pp/4k2n/p3b3/8/P1P2PPP/1P1PK1BR/2q3N1 b - - 2 17, 5k1r/pp3ppp/2q5/4N3/1nr5/2B5/PPP1QPPP/4RRK1 b - - 3 2
	
	
	return 0;
}
