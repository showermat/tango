/* 
 * File:   logic.cpp
 * Author: Matthew Schauer
 *
 * Created on July 27, 2013, 5:46 PM
 */

#include "backend.h"
#include "Bank.h"
#include "Card.h"
#include "Deck.h"

namespace backend
{
	std::string deckfname{};
	sqlite3 *db = nullptr;
	
	void cleanup()
	{
		if (db != nullptr) sqlite3_close(db);
		db = nullptr;
	}
	
	std::string deckfile()
	{
		std::string pathsep{"/"};
		std::string confdir{"tango"};
		std::string confdb{"decks.db"};
		std::string confbase{};
		char *confhome = getenv("XDG_CONFIG_HOME");
		if (confhome) confbase = confhome;
		else
		{
			char *home = getenv("HOME");
			confbase = std::string{home} + pathsep + ".config";
			// TODO If .config doesn't exit, then use ~/.tango instead
		}
		return confbase + pathsep + confdir + pathsep + confdb;
	}
	
	void batch(const std::vector<std::string> &args) try
	{
		if (args.size() < 3) throw std::runtime_error{"No action provided for batch processing"};
		if (args[2] == "debug")
		{
			if (args[3] == "load")
			{
				populate();
				std::cout << Deck::root.ncards() << " " << Card::cards().size() << "\n";
				commit();
				exit(0);
			}
		}
		else throw std::runtime_error{"Unknown batch operation " + args[2]};
		exit(0); // TODO Probably the wrong way to do this
	}
	catch (std::runtime_error &e)
	{
		std::cerr << "Error: " << e.what() << "\n";
		cleanup();
		exit(1);
	}
	
	void checksql(int code, std::string msg = "Query failed")
	{
		if (code == SQLITE_OK || code == SQLITE_DONE) return;
		msg += ": error " + util::t2s(code) + " (" + std::string{sqlite3_errstr(code)} + "): " + std::string{sqlite3_errmsg(db)};
		throw std::runtime_error{msg};
	}

	// Standin for main.  Call me to initialize stuff!
	void init(const std::vector<std::string> &args) try
	{
		deckfname = deckfile();
		// TODO Check that deck file exists.  If not, for now throw an exception
		checksql(sqlite3_open(deckfname.c_str(), &db), "Couldn't open deck database");
		if (args.size() > 1)
		{
			if (args[1] == "batch") batch(args);
			else throw std::runtime_error{"Unknown argument " + args[1]};
		}
	}
	catch (std::runtime_error e)
	{
		cleanup();
		throw;
	}

	void populate()
	{
		sqlite3_stmt *stmt;
		if (db == nullptr) throw std::runtime_error{"Database connection unexpectedly closed"};
		// TODO Check if DB is locked for editing
		
		checksql(sqlite3_prepare_v2(db, "select `name` from `deck`", -1, &stmt, nullptr), "Failed to fetch deck names");
		while (sqlite3_step(stmt) == SQLITE_ROW) Deck::add(std::string{(const char *) sqlite3_column_text(stmt, 0)});
		sqlite3_finalize(stmt);
		
		checksql(sqlite3_prepare_v2(db, "select `id`, `deck`, `offset`, `interval`, `status` from `card`", -1, &stmt, nullptr), "Failed to fetch cards");
		while (sqlite3_step(stmt) == SQLITE_ROW)
		{
			int id{sqlite3_column_int(stmt, 0)};
			std::string deckname{(const char *) sqlite3_column_text(stmt, 1)};
			int offset{sqlite3_column_int(stmt, 2)};
			int interval{sqlite3_column_int(stmt, 3)};
			Card::Status status{(Card::Status) sqlite3_column_int(stmt, 4)};
			std::unordered_map<std::string, std::string> fields{};
			sqlite3_stmt *stmt2;
			checksql(sqlite3_prepare_v2(db, "select `field`, `value` from `field` where `card` = ?", -1, &stmt2, nullptr), "Failed to fetch fields");
			checksql(sqlite3_bind_int(stmt2, 1, id), "Failed to bind deck name");
			while (sqlite3_step(stmt2) == SQLITE_ROW)
			{
				std::string field{(const char *) sqlite3_column_text(stmt2, 0)};
				std::string value{(const char *) sqlite3_column_text(stmt2, 1)};
				fields[field] = value;
			}
			sqlite3_finalize(stmt2);
			Card::add(Deck::get(deckname), id, fields, offset, interval, status, 0, true);
		}
		sqlite3_finalize(stmt);
		
		checksql(sqlite3_prepare_v2(db, "select `deck`, `name` from `character_category`", -1, &stmt, nullptr), "Failed to set up character categories"); // Using this for ordering is probably a bad idea because I don't think SQLite guarantees consistent ordering of its rows.
		while (sqlite3_step(stmt) == SQLITE_ROW)
		{
			std::string deckname{(const char *) sqlite3_column_text(stmt, 0)};
			std::string secname{(const char *) sqlite3_column_text(stmt, 1)};
			Deck::get(deckname).bank().addsect(secname);
		}
		sqlite3_finalize(stmt);
		
		checksql(sqlite3_prepare_v2(db, "select `deck`, `category`, `character`, `active`, `offset`, `count` from `character`", -1, &stmt, nullptr), "Failed to fetch characters");
		while (sqlite3_step(stmt) == SQLITE_ROW)
		{
			std::string deckname{(const char *) sqlite3_column_text(stmt, 0)};
			std::string category{(const char *) sqlite3_column_text(stmt, 1)};
			std::string character{(const char *) sqlite3_column_text(stmt, 2)};
			bool active{(bool) sqlite3_column_int(stmt, 3)};
			int offset{sqlite3_column_int(stmt, 4)};
			int count{sqlite3_column_int(stmt, 5)};
			Bank &b = Deck::get(deckname).bank();
			b.add(category, character);
			if (active) b.enable(character, offset, count, true);
		}
		sqlite3_finalize(stmt);
		//for (const Card &card : Card::cards()) for (std::string field : Card::fieldnames()) if (! card.hasfield(field)) throw std::runtime_error{"Card " + util::t2s(card.id()) + " missing field " + field};
		Deck::rebuild_all();
	}
	
	void early_populate()
	{
		sqlite3_stmt *stmt;
		if (db == nullptr) throw std::runtime_error{"Database connection unexpectedly closed"};
		
		checksql(sqlite3_prepare_v2(db, "select `name` from `fieldname`", -1, &stmt, nullptr), "Failed to fetch field names");
		while (sqlite3_step(stmt) == SQLITE_ROW)
		{
			std::string fieldname{(const char *) sqlite3_column_text(stmt, 0)};
			Card::fieldnames_.push_back(fieldname);
			Card::deffields[fieldname] = "";
		}
		sqlite3_finalize(stmt);
	}
	
	void commit()
	{
		//sqlite3_stmt *stmt;
		if (db == nullptr) throw std::runtime_error{"Database connection unexpectedly closed"};	
	}
	
	void transac_begin()
	{
		checksql(sqlite3_exec(db, "begin", 0, 0, 0));
	}
	
	void transac_end()
	{
		checksql(sqlite3_exec(db, "commit", 0, 0, 0));
	}
	
	void card_update(const Card &card)
	{
		sqlite3_stmt *stmt;
		if (db == nullptr) throw std::runtime_error{"Database connection unexpectedly closed"};
	
		bool newcard = false;
		checksql(sqlite3_prepare_v2(db, "select * from `card` where `id` = ?", -1, &stmt, nullptr));
		checksql(sqlite3_bind_int(stmt, 1, card.id()));
		if (sqlite3_step(stmt) != SQLITE_ROW) newcard = true;
		sqlite3_finalize(stmt);
		
		if (newcard) checksql(sqlite3_prepare_v2(db, "insert into `card` (`deck`, `offset`, `interval`, `status`, `id`) values (?, ?, ?, ?, ?)", -1, &stmt, nullptr));
		else checksql(sqlite3_prepare_v2(db, "update `card` set `deck` = ?, `offset` = ?, `interval` = ?, `status` = ? where `id` = ?", -1, &stmt, nullptr));
		
		std::string name = card.deck()->canonical();
		checksql(sqlite3_bind_text(stmt, 1, name.c_str(), -1, nullptr));
		checksql(sqlite3_bind_int(stmt, 2, card.offset()));
		checksql(sqlite3_bind_int(stmt, 3, card.delay()));
		checksql(sqlite3_bind_int(stmt, 4, (int) card.status()));
		checksql(sqlite3_bind_int(stmt, 5, card.id()));
		checksql(sqlite3_step(stmt));
		sqlite3_finalize(stmt);
	}
	
	void card_edit(const Card &card, const std::string &field)
	{
		std::string value = card.field(field); // The fact that this is necessary may indicate deeper issues with memory management
		sqlite3_stmt *stmt;
		if (db == nullptr) throw std::runtime_error{"Database connection unexpectedly closed"};
		
		checksql(sqlite3_prepare_v2(db, "delete from `field` where `card` = ? and `field` = ?", -1, &stmt, nullptr));
		checksql(sqlite3_bind_int(stmt, 1, card.id()));
		checksql(sqlite3_bind_text(stmt, 2, field.c_str(), -1, nullptr));
		checksql(sqlite3_step(stmt));
		sqlite3_finalize(stmt);
		
		checksql(sqlite3_prepare_v2(db, "insert into `field` (`card`, `field`, `value`) values (?, ?, ?)", -1, &stmt, nullptr));
		checksql(sqlite3_bind_int(stmt, 1, card.id()));
		checksql(sqlite3_bind_text(stmt, 2, field.c_str(), -1, nullptr));
		checksql(sqlite3_bind_text(stmt, 3, value.c_str(), -1, nullptr));
		checksql(sqlite3_step(stmt));
		sqlite3_finalize(stmt);
	}
	
	void card_del(const Card &card)
	{
		sqlite3_stmt *stmt;
		if (db == nullptr) throw std::runtime_error{"Database connection unexpectedly closed"};
		
		checksql(sqlite3_prepare_v2(db, "delete from `card` where `id` = ?", -1, &stmt, nullptr));
		checksql(sqlite3_bind_int(stmt, 1, card.id()));
		checksql(sqlite3_step(stmt));
		sqlite3_finalize(stmt);
		
		checksql(sqlite3_prepare_v2(db, "delete from `field` where `card` = ?", -1, &stmt, nullptr));
		checksql(sqlite3_bind_int(stmt, 1, card.id()));
		checksql(sqlite3_step(stmt));
		sqlite3_finalize(stmt);
	}
	
	void deck_edit(const Deck &deck, std::string oldname)
	{
		if (! deck.explic()) return;
		sqlite3_stmt *stmt;
		if (db == nullptr) throw std::runtime_error{"Database connection unexpectedly closed"};
		
		std::string name = (oldname != "" ? oldname : deck.canonical());
		bool newdeck = false;
		checksql(sqlite3_prepare_v2(db, "select * from deck where `name` = ?", -1, &stmt, nullptr));
		checksql(sqlite3_bind_text(stmt, 1, name.c_str(), -1, nullptr));
		if (sqlite3_step(stmt) != SQLITE_ROW) newdeck = true;
		sqlite3_finalize(stmt);
		
		if (newdeck) checksql(sqlite3_prepare_v2(db, "insert into `deck` (`name`) values (?)", -1, &stmt, nullptr));
		else
		{
			checksql(sqlite3_prepare_v2(db, "update `deck` set `name` = ? where `name` = ?", -1, &stmt, nullptr));
			checksql(sqlite3_bind_text(stmt, 2, name.c_str(), -1, nullptr));
		}
		checksql(sqlite3_bind_text(stmt, 1, deck.canonical().c_str(), -1, nullptr));
		checksql(sqlite3_step(stmt));
		sqlite3_finalize(stmt);
	}
	
	void deck_del(const Deck &deck)
	{
		sqlite3_stmt *stmt;
		if (db == nullptr) throw std::runtime_error{"Database connection unexpectedly closed"};
		
		checksql(sqlite3_prepare_v2(db, "delete from `deck` where `name` = ?", -1, &stmt, nullptr));
		checksql(sqlite3_bind_text(stmt, 1, deck.canonical().c_str(), -1, nullptr));
		checksql(sqlite3_step(stmt));
		sqlite3_finalize(stmt);
	}
	
	void bank_edit(const Deck &deck, const std::string &character, bool active, int offset, int count, std::string olddeck)
	{
		//std::cout << "Update " << character << " in deck " << deck.canonical() << " from deck " << olddeck << " to active = " << active << ", offset = " << offset << ", count = " << count << "\n";
		sqlite3_stmt *stmt;
		if (db == nullptr) throw std::runtime_error{"Database connection unexpectedly closed"};
		
		if (olddeck != "" && olddeck != deck.canonical())
		{
			std::string newname{deck.canonical()};
			
			checksql(sqlite3_prepare_v2(db, "update `character` set `deck` = ? where `deck` = ?", -1, &stmt, nullptr));
			checksql(sqlite3_bind_text(stmt, 1, newname.c_str(), -1, nullptr));
			checksql(sqlite3_bind_text(stmt, 2, olddeck.c_str(), -1, nullptr));
			checksql(sqlite3_step(stmt));
			sqlite3_finalize(stmt);

			checksql(sqlite3_prepare_v2(db, "update `character_category` set `deck` = ? where `deck` = ?", -1, &stmt, nullptr));
			checksql(sqlite3_bind_text(stmt, 1, newname.c_str(), -1, nullptr));
			checksql(sqlite3_bind_text(stmt, 2, olddeck.c_str(), -1, nullptr));
			checksql(sqlite3_step(stmt));
			sqlite3_finalize(stmt);
		}
		
		int i = 1;
		if (active)
		{
			checksql(sqlite3_prepare_v2(db, "update `character` set `active` = '1', `offset` = ?, `count` = ? where `deck` = ? and `character` = ?", -1, &stmt, nullptr));
			checksql(sqlite3_bind_int(stmt, i++, offset));
			checksql(sqlite3_bind_int(stmt, i++, count));
		}
		else checksql(sqlite3_prepare_v2(db, "update `character` set `active` = '0' where `deck` = ? and `character` = ?", -1, &stmt, nullptr));
		checksql(sqlite3_bind_text(stmt, i++, deck.canonical().c_str(), -1, nullptr));
		checksql(sqlite3_bind_text(stmt, i++, character.c_str(), -1, nullptr));
		checksql(sqlite3_step(stmt));
		sqlite3_finalize(stmt);
	}
}
