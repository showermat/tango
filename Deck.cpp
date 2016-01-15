/* 
 * File:   Deck.cpp
 * Author: matt
 * 
 * Created on July 27, 2013, 6:20 PM
 */

#include "Deck.h"

std::list<Deck> Deck::decks_{};
int Deck::curstep = 0;
Deck Deck::root{-1, "", true, nullptr}; // TODO Use the SetItemTypes here to set default set types.  Make a static function: set_default(sit, sit)...
int Deck::decknum_ = 1;
std::default_random_engine Deck::rand_{(long unsigned int) std::chrono::system_clock::now().time_since_epoch().count()};

void Deck::step(int offset)
{
	curstep += offset;
	backend::set_step(curstep);
	for (Deck &d : decks_) d.build();
}

Deck::Deck(int id, std::string name, bool explic, Deck *parent) : name_{name}, id_{id}, explicit_{explic}, cards_{}, parent_{parent}, children_{}, disp_{Set::defdisp() /* TODO */}, sets_{}, bank_{this, "Expression" /* TODO */}, curset_{nullptr}, valid_{true}
{
	//if (parent == nullptr) explicit_ = true;
	for (Set::SetType type : Set::settypes()) sets_.insert(std::make_pair(type, Set{this, type}));
}

Deck &Deck::ensure(std::string name, bool expl)
{
	if (name == "") return root;
	if (exists(name))
	{
		Deck &deck = *std::find_if(decks_.begin(), decks_.end(), [name](const Deck &d) { return d.canonical() == name; });
		if (expl && ! deck.explicit_) deck.edit(name, expl);
		return deck;
	}
	else
	{
		Deck *parent;
		std::string parentname = util::dirname(name);
		ensure(parentname, false);
		if (parentname == "") parent = &root;
		else parent = &*std::find_if(decks_.begin(), decks_.end(), [parentname](const Deck &d) { return d.canonical() == parentname; });
		decks_.emplace_back(Deck{decknum_++, util::basename(name), expl, parent});
		parent->add_child(&decks_.back());
		return decks_.back();
	}		
}

Deck &Deck::add(std::string name, bool fromdb)
{
	if (name == "") return root;
	Deck &ret = ensure(name, true);
	if (! fromdb && ret.explicit_) backend::deck_edit(ret);
	return ret;
}

Deck &Deck::get(std::string name)
{
	if (name == "") return root;
	return ensure(name, false);
}

void Deck::del(Deck &deck)
{
	if (deck == root || ! deck.valid_) return;
	deck.valid_ = false;
	bool explic = deck.explicit_;
	Deck *p = deck.parent_;
	for (std::unordered_set<Card *>::iterator iter = deck.cards_.begin(); iter != deck.cards_.end(); iter = deck.cards_.begin()) Card::del(**iter, true, true); // Otherwise the cards are deleted by ~Deck(), which is instructed not to propagate to the database
	if (explic) backend::deck_del(deck);
	decks_.erase(std::find_if(decks_.begin(), decks_.end(), [&deck](const Deck &d) { return deck.canonical() == d.canonical(); }));
	if (p && p->cards_.size() == 0 && p->children_.size() == 0 && ! p->explicit_) del(*p);
	std::list<Deck>::iterator iterd{};
	while ((iterd = std::find_if(decks_.begin(), decks_.end(), [&deck](const Deck &d) { return d.parent_ == &deck; })) != decks_.end()) decks_.erase(iterd);
	if (p) p->del_child(&deck, true);
}

std::string Deck::freename()
{
	int num;
	for (num = 1; exists("Untitled " + util::t2s<int>(num)); num++);
	return "Untitled " + util::t2s<int>(num);
}

std::string Deck::canonical() const
{
	if (parent_ == nullptr) return "";
	if (parent_ == &root) return name_;
	return parent_->canonical() + "/" + name_;
}

int Deck::totsize() const
{
	int ret = cards_.size();
	for (const Deck *d : children_) ret += d->totsize();
	return ret;
}

void Deck::commit(std::string oldname) const
{
	std::string thisoldname = oldname + "/" + name_;
	if (explicit_) backend::deck_edit(*this, thisoldname);
	for (const Card *c : cards_) backend::card_update(*c);
	for (const Deck *d : children_) d->commit(thisoldname);
}

void Deck::build()
{
	for (std::pair<const Set::SetType, Set> &s : sets_)
	{
		s.second.clear();
		s.second.empty();
	}
	bank().clear();
	for (Card *c : cards_)
	{
		if (c->due(0))
		{
			sets_.at(Set::SetType::NORMAL).add(c);
			if (bank().check(c) && sets_.at(Set::SetType::KANJI).size() < sets_.at(Set::SetType::KANA).size()) sets_.at(Set::SetType::KANJI).add(c); // Ensure kanji deck is not larger than kana deck
			else sets_.at(Set::SetType::KANA).add(c);
		}
		if (c->avail()) sets_.at(Set::SetType::ALL).add(c);
	}
	for (std::pair<const Set::SetType, Set> &s : sets_) s.second.shuffle();
}

bool Deck::edit(std::string name, bool explic) // This still probably doesn't work quite right if you change whether the deck is explicit
{
	if (name == canonical() && explic == explicit_) return true;
	bool move = (name != canonical() && name != "");
	std::string dest = move ? name : canonical();
	std::string parent = util::dirname(dest);
	if (move && exists(dest)) return false;
	//if (explic == false && ! exists(parent)) return false; // TODO Figure out implicit parent decks
	std::string oldname = canonical();
	if (move)
	{
		name_ = util::basename(dest);
		parent_->del_child(this);
		parent_ = &get(parent);
		parent_->add_child(this);
		backend::transac_begin();
		for (const Card *c : cards_) backend::card_update(*c);
		for (const Deck *d : children_) d->commit(oldname);
		backend::transac_end();
		if (explicit_) bank_.commit(oldname);
	}
	if (explicit_ && explic) backend::deck_edit(*this, oldname);
	else if (explicit_ && ! explic) backend::deck_del(*this);
	else if (! explicit_ && explic) disp_ = disp();
	explicit_ = explic;
	build();
	return true;
}

std::vector<std::string> Deck::vectorize(const std::vector<coldesc> &colspec)
{
	std::vector<std::string> ret{};
	for (coldesc col : colspec)
	{
		if (col.title == "Name") ret.push_back(canonical());
		else if (col.title == "Cards") ret.push_back(util::t2s<int>(cards_.size()));
		else if (col.title == "Due") ret.push_back(util::t2s<int>(sets_.at(Set::SetType::NORMAL).size()));
		else if (col.title == "Explicit") ret.push_back(explicit_ ? "true" : "false");
		else ret.push_back("NULL");
	}
	return ret;
}

void Deck::delcard(Card &c, bool refresh)
{
	s_clear();
	cards_.erase(std::find(cards_.begin(), cards_.end(), &c));
	if (cards_.size() == 0 && children_.size() == 0 && ! explicit_)
	{
		del(*this);
		return;
	}
	if (refresh) build();
}

/*void Deck::shift(int diff)
{
	backend::transac_begin();
	for (Card *c : cards_) c->shift(diff);
	backend::transac_end();
	backend::transac_begin();
	bank_.shift(diff);
	backend::transac_end();
	build();
}*/

void Deck::remove()
{
	valid_ = false;
	for (std::unordered_set<Card *>::iterator iter = cards_.begin(); iter != cards_.end(); iter = cards_.begin()) Card::del(**iter, false);
	if (parent_) parent_->del_child(this, false);
	//std::list<int, Card>::iterator iterc{};
	std::list<Deck>::iterator iterd{};
	//while ((iterc = std::find_if(Card::cards().begin(), Card::cards().end(), [this](const std::pair<const int, Card> &c) { return c.second.deck() == this; })) != Card::cards().end()) Card::del(*iterc);
	while ((iterd = std::find_if(decks_.begin(), decks_.end(), [this](const Deck &d) { return d.parent_ == this; })) != decks_.end()) decks_.erase(iterd);
}
