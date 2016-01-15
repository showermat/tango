/* 
 * File:   Deck.h
 * Author: matt
 *
 * Created on July 27, 2013, 6:19 PM
 */

#ifndef DECK_H
#define	DECK_H

#include <string>
#include <sstream>
#include <unordered_set>
#include <queue>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <cassert>
#include "Bank.h"
#include "Set.h"
#include "coldesc.h"

class Deck
{
private:
	static std::list<Deck> decks_;
	static int decknum_;
	static std::default_random_engine rand_;
	static Deck &ensure(std::string name, bool expl);
public:
	static Deck root;
	static int curstep; // Maybe this should be private
	static bool exists(std::string deck) { return std::find_if(decks_.begin(), decks_.end(), [&deck](const Deck &d) { return d.canonical() == deck; }) != decks_.end(); }
	static Deck &add(std::string name, bool fromdb = false);
	static Deck &get(std::string name);
	static std::list<Deck> &decks() { return decks_; }
	static void del(Deck &deck);
	static std::string freename();
	static void rebuild_all() { for (Deck &d : decks_) d.build(); }
	//static void shift_all(int diff) { for (Deck &d : decks_) d.shift(diff); };
	static void step(int offset);
	static void printtree(Deck *d = &root, std::string prefix = "") // For debug
	{
		std::cerr << prefix << d << " " << d->name_ << "\n";
		for (Deck *child : d->children_) printtree(child, prefix + "  ");
		if (prefix == "") std::cerr << "\n";
	}
private:
	std::string name_;
	int id_;
	bool explicit_;
	std::unordered_set<Card *> cards_;
	Deck *parent_;
	std::unordered_set<Deck *> children_;
	std::unordered_map<Set::SetType, std::unordered_map<Set::DispType, std::unordered_set<std::vector<Card::Field>, Set::vfhash>, Set::dthash>, Set::sthash> disp_;
	std::unordered_map<Set::SetType, Set> sets_;
	Bank bank_;
	Set *curset_;
	bool valid_;
	Deck(int id, std::string name, bool explic, Deck *parent);
	int totsize() const;
	void commit(std::string oldname) const;
	void remove();
	std::unordered_map<Set::SetType, std::unordered_map<Set::DispType, std::unordered_set<std::vector<Card::Field>, Set::vfhash>, Set::dthash>, Set::sthash> disp() { if (explicit_) return disp_; return parent_->disp(); };
public:
	Deck() = delete;
	Deck(const Deck& orig) = delete;
	Deck(Deck&& orig) : name_{orig.name_}, id_{orig.id_}, explicit_{orig.explicit_}, cards_{std::move(orig.cards_)}, parent_{orig.parent_}, children_{std::move(orig.children_)}, disp_{orig.disp_}, sets_{std::move(orig.sets_)}, bank_{orig.bank_}, curset_{orig.curset_}, valid_{true}
	{
		orig.valid_ = false;
		for (std::pair<const Set::SetType, Set> &pair : sets_) pair.second.deck(this);
		bank_.deck(this);
	}
	Deck operator =(const Deck& orig) = delete;
	virtual ~Deck() { if (valid_) remove(); }
	
	std::string name() const { return name_; }
	std::string canonical() const;
	Deck *parent() const { return parent_; }
	std::unordered_set<Deck *> children() const { return children_; }
	int id() const { return id_; }
	int size() const { return cards_.size(); }
	const std::unordered_set<Card *> &cards() const { return cards_; }
	bool explic() const { return explicit_; }
	Deck *inherited() { for (Deck *d = this; d != nullptr; d = d->parent_) if (d->explicit_) return d; return &root; }
	bool has(Card &card) const { return card.deck() == this; }
	friend bool operator ==(const Deck &a, const Deck &b) { return a.id_ == b.id_; }
	Set &set(Set::SetType type) { return sets_.at(type); }
	Bank &bank() { return bank_; }
	std::unordered_map<Set::DispType, std::unordered_set<std::vector<Card::Field>, Set::vfhash>, Set::dthash> disp(Set::SetType type) { return disp().at(type); }
	std::unordered_map<Set::SetType, Set> &sets() { return sets_; }
	std::vector<std::string> vectorize(const std::vector<coldesc> &colspec);
	int ncards() { int n = cards_.size(); for (Deck *d : children_) n += d->ncards(); return n; }

	//void shift(int diff);
	void add_child(Deck *d, bool refresh = true) { children_.insert(d); if (refresh) build(); }
	void del_child(Deck *d, bool refresh = true) { children_.erase(d); if (refresh) build(); }
	bool edit(std::string name, bool explic);
	void build();
	void s_clear() { for (std::pair<const Set::SetType, Set> &s : sets_) s.second.clear(); }
	void addcard(Card &c, bool refresh = true) { cards_.insert(&c); if (refresh) build(); }
	void delcard(Card &c, bool refresh = true);
};

namespace std { template <> struct hash<Deck> { size_t operator ()(const Deck &x) const { return static_cast<size_t>(x.id()); } }; }

#endif	/* DECK_H */

