/* 
 * File:   Set.cpp
 * Author: matt
 * 
 * Created on December 9, 2013, 3:04 PM
 */

#include "Set.h"
#include "Deck.h"

const std::vector<Set::SetType> Set::settypes() { return std::vector<Set::SetType>{Set::SetType::NORMAL, Set::SetType::ALL, Set::SetType::KANJI, Set::SetType::KANA}; }
std::unordered_map<Set::SetType, std::unordered_map<Set::DispType, std::unordered_set<std::vector<Card::Field>, Set::vfhash>, Set::dthash>, Set::sthash> Set::defdisp()
{
	return std::unordered_map<Set::SetType, std::unordered_map<Set::DispType, std::unordered_set<std::vector<Card::Field>, Set::vfhash>, Set::dthash>, Set::sthash> { // Your worst nightmare
	{Set::SetType::NORMAL,
		{{Set::DispType::FRONT, {{Card::Field::MEANING}, {Card::Field::FURIGANA}}},
		{Set::DispType::BACK, {{Card::Field::FURIGANA, Card::Field::MEANING}}},
		{Set::DispType::HINT, {{Card::Field::HIRAGANA}}}}},
	{Set::SetType::ALL,
		{{Set::DispType::FRONT, {{Card::Field::MEANING}, {Card::Field::FURIGANA}}},
		{Set::DispType::BACK, {{Card::Field::FURIGANA, Card::Field::MEANING}}},
		{Set::DispType::HINT, {{Card::Field::HIRAGANA}}}}},
	{Set::SetType::KANJI,
		{{Set::DispType::FRONT, {{Card::Field::MEANING}}},
		{Set::DispType::BACK, {{Card::Field::FURIGANA, Card::Field::MEANING}}},
		{Set::DispType::HINT, {{Card::Field::HIRAGANA}}}}},
	{Set::SetType::KANA,
		{{Set::DispType::FRONT, {{Card::Field::MEANING}, {Card::Field::FURIGANA}}},
		{Set::DispType::BACK, {{Card::Field::FURIGANA, Card::Field::MEANING}}},
		{Set::DispType::HINT, {{Card::Field::HIRAGANA}}}}}
	};
}

Set::Set(Deck *deck, SetType type) : items_{}, repeats_{}, type_{type}, top_{nullptr}, deck_{deck}, displays_{deck->disp(type)}, curdisp_{} { deck_->build(); shuffle(); }

std::string Set::canonical() const
{
	return deck_->canonical() + ":" + st2str(type_);
}

void Set::clear()
{
	if (! top_) return;
	for (Deck *d = top_->deck(); d != &Deck::root; d = d->parent()) d->set(type_).top_ = nullptr; // TODO Check
	top_ = nullptr;
}

Card &Set::top()
{
	if (top_) return *top_;
	std::vector<Set *> viable{};
	if (items_.size() > 0) viable.push_back(this);
	for (Deck *d : deck_->children()) if (d->set(type_).size(false) > 0) viable.push_back(&d->set(type_));
	if (viable.size() == 0)
	{
		if (repeats_.size() > 0) viable.push_back(this);
		for (Deck *d : deck_->children()) if (d->set(type_).size(true) > 0) viable.push_back(&d->set(type_));	
	}
	Set *src = viable[rand() % viable.size()];
	if (src == this)
	{
		if (src->items_.size() > 0) { top_ = src->items_.front(); src->items_.pop_front(); }
		else if (src->repeats_.size() > 0) { top_ = src->repeats_.front(); src->repeats_.pop_front(); }
		else throw std::runtime_error{"Tried to get card out of empty deck"};
	}
	else top_ = &src->top();
	for (std::pair<DispType, std::unordered_set<std::vector<Card::Field>, vfhash>> pair : displays_)
	{
		std::unordered_set<std::vector<Card::Field>, vfhash>::iterator iter = pair.second.begin();
		std::advance(iter, rand() % pair.second.size());
		curdisp_[pair.first] = *iter;
	}
	return *top_;
}

std::string Set::disptop(DispType type)
{
	Card &c_top = top();
	return c_top.display(curdisp_.at(type));
}

void Set::shuffle()
{
	clear();
	std::list<Card *> temp{};
	while (items_.size())
	{
		temp.push_back(items_.front());
		items_.pop_front();
	}
	while(temp.size())
	{
		std::list<Card *>::iterator iter = std::next(temp.begin(), rand() % temp.size());
		items_.push_back(*iter);
		temp.erase(iter);
	}
}

int Set::size(bool repeats) const
{
	int ret = items_.size();
	if (repeats) ret += repeats_.size();
	if (top_ != nullptr && top_->deck() == deck_) ret++;
	for (Deck *d : deck_->children()) ret += d->set(type_).size(repeats);
	//std::cout << "Set " << canonical() << " size " << ret << " top " << (top_ ? util::t2s(top_->id()) : "nil") << "\n\tItems:\n";
	//for (Card *c : items_) std::cout << "\t\t" << c->id() << "\n";
	//std::cout << "\tRepeats:\n";
	//for (Card *c : repeats_) std::cout << "\t\t" << c->id() << "\n";
	return ret;
}

void Set::remove(Card *card)
{
	std::deque<Card *>::iterator iter;
	iter = std::find(items_.begin(), items_.end(), card);
	if (iter != items_.end())
	{
		items_.erase(iter);
		return;
	}
	iter = std::find(repeats_.begin(), repeats_.end(), card);
	if (iter != repeats_.end()) repeats_.erase(iter);
}

void Set::update(Card::UpdateType ut)
{
	if (top_ == nullptr) throw std::runtime_error{"Tried to update inactive set"};
	top_->update(ut);
	if (type_ == SetType::KANJI) top_->deck()->bank().update(top_, ut);
	if (ut == Card::UpdateType::BURY || top_->due(0)) top_->deck()->set(type_).repeats_.push_back(top_);
	else for (std::unordered_map<Set::SetType, Set>::iterator iter = top_->deck()->sets().begin(); iter != top_->deck()->sets().end(); iter++) iter->second.remove(top_);
	clear();
}
