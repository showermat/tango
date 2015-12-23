/* 
 * File:   logic.h
 * Author: matt
 *
 * Created on July 28, 2013, 4:17 PM
 */

#ifndef LOGIC_H
#define	LOGIC_H

#include <vector>
#include <list>
#include <iterator>
#include <unordered_map>
#include <string>
#include <fstream>
#include <exception>
#include <stdexcept>
#include <algorithm>
#include <iostream> // TODO Remove after debugging
#include <stdlib.h>
#include <sqlite3.h>
#include "util.h"

class Card;
class Deck;

namespace backend
{
	extern sqlite3 *db;
	
	void init(const std::vector<std::string> &args);
	void db_setup(const std::string &fname);
	void early_populate();
	void populate();
	void commit();
	void cleanup();
	void transac_begin();
	void transac_end();
	void card_update(const Card &card);
	void card_edit(const Card &card, const std::string &field);
	void card_del(const Card &card);
	void deck_edit(const Deck &deck, std::string oldname = "");
	void deck_del(const Deck &deck);
	void bank_edit(const Deck &deck, const std::string &character, bool active, int offset, int count, std::string olddeck = "");
}

#endif	/* LOGIC_H */

