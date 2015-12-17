/* 
 * File:   gui.cpp
 * Author: Matthew Schauer
 *
 * Created on July 27, 2013, 5:46 PM
 */

#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/treectrl.h>
#include <wx/stattext.h>
#include <wx/dataview.h>
#include <wx/srchctrl.h>
#include <wx/html/htmlwin.h>
#include "wx/html/m_templ.h"
#include "Deck.h"
#include "Set.h"
#include "Card.h"
#include "Bank.h"
#include "coldesc.h"
#include "backend.h"
#include <algorithm>
#include <exception>
#include <stdexcept>
#include <unordered_map>
#include <queue>

#define PROGRAM "Tango"
#define VERSION "0.4"
#define AUTHOR "Matthew Schauer"
#define DATE "2015"
#define LICENSE "Apache License version 2.0"

/* TODO
 * Soon:
 * Next:
 *	Make the disp_ field in Deck persistent and allow the user to edit it
 *	First time opening Tango: Set up DB
 *	Allow non-keypad keys to be used for study as well!
 * Closing a window in front of Tango may cause it to quit as well?
 * Completing a card in repeats causes set size to decrease by 2
 * Add some actual content to the about screen
 * Preferences screen
 *	Auto-advance decks every 24 hours
 *	Display settings for root deck, inherited by implicit decks
 * 
 * Long-term:
 * Multiple readings, writings, meanings
 *	It is possible to make multiple entries for a given field on a given card in the database
 *	Could randomly choose one to display, and display all possibilities separated by "or" in the answer
 *	Makes editing more difficult
 *	What if a given reading only goes with one writing, etc?
 * Give each word a "momentum" that increases/decreases as the user answers more correct/incorrect
 * Flesh out the overview screen with graphs and so forth
 *	Add to card database fields for number of practices and number of correct/incorrect/resets/etc., and use this to calculate momentum or "score"
 * Add a preferences window allowing users to edit
 *	The maximum interval before a card is automatically marked done
 *	Option to keep track of the last date things were practiced and automatically advance the cards each time the program is opened
 *	Default settings for root deck
 * Keep track of the total number of steps taken and the number of steps since a given card was suspended
 * Filter card table by deck
 * Fill in the blanks in sample sentences
 * Single out and present problem children
 */

/******************************************************************************
 * GUI structure
 ******************************************************************************/

enum { id_menu_about, id_menu_quit, id_menu_refresh, id_notebook, id_decks_tree, id_browse_cards, id_card_add, id_card_del, id_card_find, id_browse_decks, id_deck_add, id_deck_del, id_set_type, id_browse_bank, id_offset_forward, id_offset_back };

namespace std
{
	template <> struct hash<wxDataViewItem> { size_t operator ()(const wxDataViewItem &x) const { return (size_t) x.GetID(); } };
}

class MainFrame;

class App: public wxApp
{
	virtual bool OnInit();
	virtual int OnExit();
	virtual bool OnExceptionInMainLoop();
	MainFrame *frame;
};

class MainFrame : public wxFrame
{
public:
	MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
	void populate_decktree(Deck *d = &Deck::root);
	void populate_table_cols(wxDataViewListCtrl *table, const std::vector<coldesc> &columns);
	wxVector<wxVariant> cols2wxvec(std::vector<coldesc> colspec, std::vector<std::string> cols);
	void table_updaterow(wxDataViewListCtrl *table, int row, const wxVector<wxVariant> &data);
	void pagechange();
	void showcard();
	void addcard();
	void delcard(int row);
	void stattext(const std::string &text = "");
	void setbankitem(const std::string word, bool enabled);
	void populate_cardtable(std::string filter = "");
	void populate_decktable();
	void populate_bankview();
	void refresh_views(int mode = 0xff);
	void debug_tablecheck();
	void about(wxCommandEvent &event);
	void quit(wxCommandEvent &event);
	void refresh(wxCommandEvent &event);
	void switch_deck(wxTreeEvent &event);
	void activate_deck(wxTreeEvent &event);
	void page_changed(wxNotebookEvent &event);
	void ensure_exists(const std::string &deck);
	void offset_advanced(wxCommandEvent &event);
	void offset_reversed(wxCommandEvent &event);
	void card_added(wxCommandEvent &event);
	void card_deleted(wxCommandEvent &event);
	void card_searched(wxCommandEvent &event);
	void card_edited(wxDataViewEvent &event);
	void deck_added(wxCommandEvent &event);
	void deck_deleted(wxCommandEvent &event);
	void deck_edited(wxDataViewEvent &event);
	void bankitem_set(wxHtmlLinkEvent &event);
	//void change_settype(wxCommandEvent &event);
	void keydown(wxKeyEvent &event);
	void key_view(wxKeyEvent &event);
	void close(wxCloseEvent &event);
	void err(const std::string msg);
	void except(const std::exception &e);
	
	Set *curset;
	//Set::SetType settype;
	Set::DispType disp;
	wxFont font_header;
	std::string html_pref, html_suff;
private:
	wxNotebook *notebook;
	wxPanel *panel_tree;
	wxPanel *panel_deck;
	wxPanel *panel_study;
	wxPanel *panel_cards;
	wxPanel *panel_decks;
	wxPanel *panel_bank;
	
	wxTreeCtrl *tree_decks;
	wxButton *offset_forward;
	wxButton *offset_back;
	
	wxStaticText *deck_name;
	
	wxHtmlWindow *study_view;

	wxDataViewListCtrl *browse_cards;
	std::vector<coldesc> card_columns;
	wxDataViewSpinRenderer *dropdownRenderer;
	wxButton *card_add;
	wxButton *card_del;
	wxSearchCtrl *card_find;
	
	wxDataViewListCtrl *browse_decks;
	std::vector<coldesc> deck_columns;
	wxButton *deck_add;
	wxButton *deck_del;
	
	std::unordered_map<std::string, wxTreeItemId> deckids;
	std::unordered_map<wxDataViewItem, Card *> card_rows;
	std::unordered_map<wxDataViewItem, Deck *> deck_rows;
	std::unordered_map<wxDataViewItem, std::string> bank_rows;
	
	wxHtmlWindow *bank_view;
	
	std::pair<Deck *, Set::SetType> tree2deck(wxTreeItemId id);
	Card *row2card(int row);
	int card2row(Card *card);
	int table_addcard(Card &card);
	void table_delcard(int row);
	Deck *row2deck(int row);
	int deck2row(Deck *deck);
	int table_adddeck(Deck &deck);
	void table_deldeck(int row);
	
	DECLARE_EVENT_TABLE()
};

TAG_HANDLER_BEGIN(BANKBTN, "BANKBTN")
TAG_HANDLER_PROC(tag)
{
	MainFrame *frame = (MainFrame *) m_WParser->GetWindowInterface()->GetHTMLWindow()->GetParent()->GetParent()->GetParent(); // Is there seriously no easier way to get the frame in this context?	
	wxString word = tag.GetParam(_("name"));
	bool enabled = frame->curset->deck().bank().enabled(word.ToStdString());
	wxButton *button = new wxButton{m_WParser->GetWindowInterface()->GetHTMLWindow(), wxID_ANY, word, wxDefaultPosition, wxSize{46, 46}, wxBORDER_NONE};
	button->SetFont(wxFont{24, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL});
	button->SetForegroundColour(enabled ? wxColour{_("black")} : wxColour{_("gray")});
	button->Bind(wxEVT_BUTTON, [=](wxCommandEvent &event)
	{
		bool en = ! frame->curset->deck().bank().enabled(word.ToStdString());
		button->SetForegroundColour(en ? wxColour{"black"} : wxColour{"gray"});
		frame->setbankitem(word.ToStdString(), en);
	});
	button->Show(true);
	m_WParser->GetContainer()->InsertCell(new wxHtmlWidgetCell(button, 0));
	
	return false;
}
TAG_HANDLER_END(BANKBTN)

TAGS_MODULE_BEGIN(bankbtn)
TAGS_MODULE_ADD(BANKBTN)
TAGS_MODULE_END(bankbtn)

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
	EVT_MENU(id_menu_about, MainFrame::about)
	EVT_MENU(id_menu_quit, MainFrame::quit)
	EVT_MENU(id_menu_refresh, MainFrame::refresh)
	EVT_BUTTON(id_offset_forward, MainFrame::offset_advanced)
	EVT_BUTTON(id_offset_back, MainFrame::offset_reversed)
	EVT_BUTTON(id_card_add, MainFrame::card_added)
	EVT_BUTTON(id_card_del, MainFrame::card_deleted)
	EVT_BUTTON(id_card_find, MainFrame::card_searched)
	EVT_BUTTON(id_deck_add, MainFrame::deck_added)
	EVT_BUTTON(id_deck_del, MainFrame::deck_deleted)
	//EVT_CHOICE(id_set_type, MainFrame::change_settype)
	EVT_TREE_ITEM_ACTIVATED(id_decks_tree, MainFrame::activate_deck)
	EVT_TREE_SEL_CHANGED(id_decks_tree, MainFrame::switch_deck)
	EVT_NOTEBOOK_PAGE_CHANGED(id_notebook, MainFrame::page_changed)
	EVT_DATAVIEW_ITEM_VALUE_CHANGED(id_browse_cards, MainFrame::card_edited)
	EVT_DATAVIEW_ITEM_VALUE_CHANGED(id_browse_decks, MainFrame::deck_edited)
	EVT_TEXT(id_card_find, MainFrame::card_searched)
	EVT_CLOSE(MainFrame::close)
END_EVENT_TABLE()

IMPLEMENT_APP(App)
	
bool App::OnInit() try
{
	std::vector<std::string> args;
	for (int i = 0; i < argc; i++) args.push_back(wxString{argv[i]}.ToStdString());
	backend::init(args);
	backend::early_populate(); // Load fieldnames
	frame = new MainFrame{_("Tango"), wxPoint(200, 200), wxSize(600, 400)};
	frame->Center();
	frame->Show(true);
	SetTopWindow(frame);
	frame->stattext("Loading decks...");
	backend::populate();
	frame->refresh_views();
	frame->stattext("No deck selected");
	return true;
}
catch (std::exception &e)
{
	std::cerr << "Error: " << e.what() << "\n";
	frame->except(e);
	return true;
}

int App::OnExit()
{
	return 0;
}

bool App::OnExceptionInMainLoop()
{
	frame->except(std::runtime_error{"Uncaught exception in main loop"});
	return false;
}

void MainFrame::except(const std::exception &e)
{
	wxMessageDialog message{this, _(std::string{"The program experienced a fatal exception and will now exit:\n\n"} + e.what()), _("Exception"), wxOK | wxICON_ERROR};
	message.ShowModal();
	// TODO Can we try saving decks here?
	wxExit();
}

void MainFrame::err(const std::string msg)
{
	wxMessageDialog{this, _(std::string{"Error:  "} + msg), _("Error"), wxOK | wxICON_ERROR}.ShowModal();	
}

MainFrame::MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size) try : wxFrame{NULL, -1, title, pos, size}
{
	curset = nullptr;
	//settype = Set::SetType::NORMAL; // TODO User-set
	disp = Set::DispType::FRONT;
	font_header = wxFont{-1, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD};
	html_pref = "<html><body><center>";
	html_suff = "</center></body></html>";
	
	wxMenu *menu_file = new wxMenu{};
	menu_file->Append(id_menu_about, _("&About..."));
	menu_file->AppendSeparator();
	menu_file->Append(id_menu_refresh, _("&Refresh"));
	menu_file->Append(id_menu_quit, _("Quit"));
	
	wxMenuBar *menubar = new wxMenuBar;
	menubar->Append(menu_file, _("&File"));
	SetMenuBar(menubar);
	
	CreateStatusBar();
	
	notebook = new wxNotebook{this, id_notebook};
	
	// Deck tree panel
	panel_tree = new wxPanel{notebook, -1, wxPoint(-1, -1), wxSize(-1, -1)};
	wxSizer *sizer_tree = new wxBoxSizer{wxVERTICAL};
	tree_decks = new wxTreeCtrl{panel_tree, id_decks_tree, wxDefaultPosition, wxSize(-1, -1), wxTR_HIDE_ROOT | wxTR_HAS_BUTTONS};
	sizer_tree->Add(tree_decks, 1, wxALIGN_CENTER | wxEXPAND | wxALL, 10);
	offset_forward = new wxButton{panel_tree, id_offset_forward, ">"};
	offset_back = new wxButton{panel_tree, id_offset_back, "<"};
	wxSizer *sizer_offset = new wxBoxSizer(wxHORIZONTAL);
	sizer_offset->Add(offset_back, 1, wxEXPAND | wxRIGHT, 5);
	sizer_offset->Add(offset_forward, 1, wxEXPAND | wxLEFT, 5);
	sizer_tree->Add(sizer_offset, 0, wxLEFT | wxBOTTOM | wxRIGHT | wxEXPAND, 10);
	panel_tree->SetSizerAndFit(sizer_tree);
	notebook->AddPage(panel_tree, _("Tree"), true);
	
	// Deck overview panel
	panel_deck = new wxPanel{notebook, -1, wxPoint(-1, -1), wxSize(-1, -1)};
	wxSizer *sizer_deck = new wxBoxSizer{wxVERTICAL};
	deck_name = new wxStaticText{panel_deck, -1, _(""), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER | wxST_NO_AUTORESIZE};
	deck_name->SetFont(font_header);
	sizer_deck->Add(deck_name, 0, wxALIGN_CENTER | wxEXPAND | wxALL, 10);
	panel_deck->SetSizerAndFit(sizer_deck);
	notebook->AddPage(panel_deck, _("Overview"));
	
	// Study panel
	panel_study = new wxPanel{notebook, -1, wxPoint(-1, -1), wxSize(-1, -1)};
	wxSizer *sizer_study = new wxBoxSizer{wxVERTICAL};
	study_view = new wxHtmlWindow{panel_study};
	study_view->Bind(wxEVT_KEY_UP, &MainFrame::keydown, this);
	sizer_study->Add(study_view, 1, wxALIGN_CENTER | wxEXPAND | wxALL, 10);
	panel_study->SetSizerAndFit(sizer_study);
	notebook->AddPage(panel_study, _("Study"));
	
	// Cards panel
	panel_cards = new wxPanel{notebook, -1, wxPoint(-1, -1), wxSize(-1, -1)};
	wxSizer *sizer_cards = new wxBoxSizer{wxVERTICAL};
	browse_cards = new wxDataViewListCtrl{panel_cards, id_browse_cards};
	card_columns.push_back(coldesc{coldesc::Type::STRING, "Deck", 160});
	for (std::string s : Card::fieldnames()) card_columns.push_back(coldesc{coldesc::Type::FIELD, s, 160});
	card_columns.push_back(coldesc{coldesc::Type::CHOICE, "Status:Active,Suspended,Done,Leech", 80});
	card_columns.push_back(coldesc{coldesc::Type::INT, "Offset", 80});
	card_columns.push_back(coldesc{coldesc::Type::INT, "Interval:0," + util::t2s<int>(Card::maxdelay() * 2), 80});
	populate_table_cols(browse_cards, card_columns);
	sizer_cards->Add(browse_cards, 1, wxALIGN_CENTER | wxEXPAND | wxALL, 10);
	wxSizer *sizer_card_buttons = new wxBoxSizer{wxHORIZONTAL};
	card_add = new wxButton{panel_cards, id_card_add, "Add"};
	card_del = new wxButton{panel_cards, id_card_del, "Delete"};
	card_find = new wxSearchCtrl{panel_cards, id_card_find, wxEmptyString, wxDefaultPosition, wxSize{140, 30}, wxTE_PROCESS_ENTER};
	sizer_card_buttons->Add(card_find, 0, wxALIGN_LEFT);
	sizer_card_buttons->Add(0, 0, 1);
	sizer_card_buttons->Add(card_del, 0, wxALIGN_LEFT | wxLEFT, 10);
	sizer_card_buttons->Add(card_add, 0, wxALIGN_RIGHT | wxLEFT, 10);
	sizer_cards->Add(sizer_card_buttons, 0, wxLEFT | wxBOTTOM | wxRIGHT | wxALIGN_RIGHT | wxEXPAND, 10);
	panel_cards->SetSizerAndFit(sizer_cards);
	notebook->AddPage(panel_cards, _("Cards"));
	
	// Decks panel
	panel_decks = new wxPanel{notebook, -1, wxPoint(-1, -1), wxSize(-1, -1)};
	wxSizer *sizer_decks = new wxBoxSizer{wxVERTICAL};
	browse_decks = new wxDataViewListCtrl{panel_decks, id_browse_decks};
	deck_columns.push_back(coldesc{coldesc::Type::STRING, "Name", 160});
	deck_columns.push_back(coldesc{coldesc::Type::STATIC_INT, "Cards", 40});
	deck_columns.push_back(coldesc{coldesc::Type::STATIC_INT, "Due", 40});
	deck_columns.push_back(coldesc{coldesc::Type::BOOL, "Explicit", 40});
	//deck_columns.push_back(coldesc{coldesc::Type::CHOICE, "Front:Kanji,Hiragana,Furigana,Meaning,All"});
	//deck_columns.push_back(coldesc{coldesc::Type::CHOICE, "Back:Kanji,Hiragana,Furigana,Meaning,All"});
	populate_table_cols(browse_decks, deck_columns);
	browse_decks->Bind(wxEVT_KEY_DOWN, &MainFrame::key_view, this);
	sizer_decks->Add(browse_decks, 1, wxALIGN_CENTER | wxEXPAND | wxALL, 10);
	wxSizer *sizer_deck_buttons = new wxBoxSizer{wxHORIZONTAL};
	deck_add = new wxButton{panel_decks, id_deck_add, "Add"};
	deck_del = new wxButton{panel_decks, id_deck_del, "Delete"};
	sizer_deck_buttons->Add(deck_del, 0, wxALIGN_RIGHT | wxLEFT, 10);
	sizer_deck_buttons->Add(deck_add, 0, wxALIGN_RIGHT | wxLEFT, 10);
	sizer_decks->Add(sizer_deck_buttons, 0, wxALIGN_RIGHT | wxLEFT | wxBOTTOM | wxRIGHT, 10);
	panel_decks->SetSizerAndFit(sizer_decks);
	notebook->AddPage(panel_decks, _("Decks"));
	
	// Bank panel
	panel_bank = new wxPanel{notebook, -1, wxPoint(-1, -1), wxSize(-1, -1)};
	wxSizer *sizer_bank = new wxBoxSizer{wxVERTICAL};
	bank_view = new wxHtmlWindow{panel_bank};
	sizer_bank->Add(bank_view, 1, wxEXPAND | wxALL, 10);
	panel_bank->SetSizerAndFit(sizer_bank);
	notebook->AddPage(panel_bank, _("Bank"));
}
catch(std::exception &e) { except(e); }

void MainFrame::populate_table_cols(wxDataViewListCtrl *table, const std::vector<coldesc> &columns)
{
	table->DeleteAllItems();
	table->ClearColumns();
	unsigned int i = 0;
	for (coldesc col : columns)
	{
		if (col.type == coldesc::Type::STRING || col.type == coldesc::Type::FIELD)
		{
			table->AppendTextColumn(_(col.title), wxDATAVIEW_CELL_EDITABLE, col.width, wxALIGN_LEFT, wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE);
		}
		else if (col.type == coldesc::Type::INT)
		{
			wxDataViewRenderer *int_renderer = new wxDataViewSpinRenderer{col.min, col.max};
			wxDataViewColumn *int_column = new wxDataViewColumn{_(col.title), int_renderer, i, col.width, wxALIGN_LEFT, wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE};
			table->AppendColumn(int_column);
		}
		else if (col.type == coldesc::Type::STATIC_INT)
		{
			table->AppendTextColumn(_(col.title), wxDATAVIEW_CELL_INERT, col.width, wxALIGN_LEFT, wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE);
		}
		/*else if (col.type == coldesc::Type::DATE)
		{
			wxDataViewRenderer *date_renderer = new wxDataViewDateRenderer{};
			wxDataViewColumn *date_column = new wxDataViewColumn{_(col.title), date_renderer, i, col.width, wxALIGN_LEFT, wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE};
			table->AppendColumn(date_column);
		}*/
		else if (col.type == coldesc::Type::CHOICE)
		{
			wxArrayString choices{};
			for (std::string choice : col.choices) choices.Add(_(choice));
			wxDataViewRenderer *choice_renderer = new wxDataViewChoiceRenderer{choices};
			wxDataViewColumn *choice_column = new wxDataViewColumn{_(col.title), choice_renderer, i, col.width, wxALIGN_LEFT, wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE};
			table->AppendColumn(choice_column);
		}
		else if (col.type == coldesc::Type::BOOL)
		{
			table->AppendToggleColumn(_(col.title), wxDATAVIEW_CELL_ACTIVATABLE, col.width, wxALIGN_CENTER);
		}
		i++;
	}
}

/******************************************************************************
 * Converters
 ******************************************************************************/

wxVector<wxVariant> MainFrame::cols2wxvec(std::vector<coldesc> colspec, std::vector<std::string> cols)
{
	wxVector<wxVariant> row{};
	for (unsigned int i = 0; i < cols.size(); i++)
	{
		wxVariant variant{};
		coldesc::Type type = colspec[i].type;
		if (type == coldesc::Type::STRING || type == coldesc::Type::FIELD || type == coldesc::Type::STATIC_INT) variant = wxVariant{_(cols[i])};
		//else if (type == coldesc::Type::DATE) variant = wxVariant{wxDateTime{date{cols[i]}.mktime()}};
		else if (type == coldesc::Type::CHOICE) variant = wxVariant{_(cols[i])};
		else if (type == coldesc::Type::INT) variant = wxVariant{util::s2t<int>(cols[i])};
		else if (type == coldesc::Type::BOOL) variant = wxVariant{cols[i] == "true" ? true : false};
		row.push_back(variant);
	}
	return row;
}

std::pair<Deck *, Set::SetType> MainFrame::tree2deck(wxTreeItemId id)
{
	for (std::pair<std::string, wxTreeItemId> kvpair : deckids) if (kvpair.second == id)
	{
		Deck *d = &Deck::get(util::strto(kvpair.first, ":"));
		std::string ststr = util::strto(kvpair.first, ":");
		Set::SetType st = Set::str2st((ststr == "") ? "Normal" : ststr);
		return std::make_pair(d, st);
	}
	return std::make_pair(nullptr, Set::SetType::ALL);
}

Card *MainFrame::row2card(int row)
{
	wxDataViewItem item = browse_cards->RowToItem(row);
	if (! item.IsOk() || ! card_rows.count(item)) return nullptr;
	return card_rows[item];
}

int MainFrame::card2row(Card *card)
{
	for (std::pair<wxDataViewItem, Card *> pair : card_rows) if (pair.second == card) return browse_cards->ItemToRow(pair.first);
	return -1;
}

Deck *MainFrame::row2deck(int row)
{
	wxDataViewItem item = browse_decks->RowToItem(row);
	if (! item.IsOk() || ! deck_rows.count(item)) return nullptr;
	return deck_rows[item];
}

int MainFrame::deck2row(Deck *deck)
{
	for (std::pair<wxDataViewItem, Deck *> pair : deck_rows) if (pair.second == deck) return browse_decks->ItemToRow(pair.first);
	return -1;
}

void MainFrame::table_updaterow(wxDataViewListCtrl *table, int row, const wxVector<wxVariant> &data)
{
	for (unsigned int i = 0; i < data.size(); i++) table->SetValue(data[i], row, i);
}

void MainFrame::addcard()
{
	Deck *deck;
	if (curset == nullptr) deck = &Deck::get(Deck::freename());
	else deck = &curset->deck();
	try
	{
		Card &card = Card::add(*deck);
		table_addcard(card);
		refresh_views(0x6); // Update deck tree, deck table
		browse_cards->SelectRow(card2row(&card));
		browse_cards->EnsureVisible(browse_cards->RowToItem(card2row(&card)));
	}
	catch (std::runtime_error &e)
	{
		err(e.what());
		refresh_views(0x6);
	}
}

void MainFrame::delcard(int row)
{
	try
	{
		Card::del(*row2card(row));
		table_delcard(row);
		refresh_views(0x6); // Update deck tree, deck table
	}
	catch (std::runtime_error &e)
	{
		err(e.what());
		refresh_views(0x6);
	}
}

/******************************************************************************
 * GUI updaters
 ******************************************************************************/

void MainFrame::showcard()
{
	if (! curset) study_view->SetPage(_(html_pref + "(No deck selected)" + html_suff));
	else if (curset->size() == 0) study_view->SetPage(_(html_pref + "(No cards in current set)" + html_suff));
	//else study_view->SetPage(_(html_pref + curset->top().display(cardback ? curset->deck().type_back() : curset->type_front()) + html_suff));
	else study_view->SetPage(_(html_pref + curset->disptop(disp) + html_suff));
	stattext();
}

void MainFrame::pagechange()
{
	int page = notebook->GetSelection();
	switch (page)
	{
		case 0: // Decks
			populate_decktree();
			break;
		case 1: // Deck
			if (! curset) deck_name->SetLabel(_("(No deck selected)"));
			else deck_name->SetLabel(_(curset->canonical()));
			break;
		case 2: // Study
			disp = Set::DispType::FRONT;
			showcard();
			break;
		case 3: // Cards
			//populate_browsetable();
			break;
		case 4: // Decks
			break;
		case 5: // Bank
			refresh_views(0x8);
			break;
	}
	stattext();
}

void MainFrame::stattext(const std::string &text)
{
	if (text != "") SetStatusText(_(text));
	else if (! curset) SetStatusText(_("(No deck selected)"));
	else
	{
		int page = notebook->GetSelection();
		if (page == 0 || page == 1 || page == 3) SetStatusText(_(curset->canonical() + "  |  " + util::t2s<int>(curset->size())));
		else if (page == 2)
		{
			if (curset->size() > 0) SetStatusText(_(curset->canonical() + "  |  " + util::t2s<int>(curset->size()) + "  |  " + util::t2s<int>(curset->top().delay())));
			else SetStatusText(_(curset->canonical() + "  |  0"));
		}
	}
}

void MainFrame::debug_tablecheck()
{
	for (const std::pair<wxDataViewItem, Card *> &pair : card_rows)
	{
		wxVariant var{};
		browse_cards->GetValue(var, browse_cards->ItemToRow(pair.first), 1);
		//std::cerr << var.GetString().ToStdString() << "\t" << pair.second->field("Expression") << "\n";
		if (var.GetString().ToStdString() != pair.second->field("Expression"))
		{
			std::cerr << "Card mapping mismatch:  " << var.GetString().ToStdString() << " vs " << pair.second->field("Expression") << "\n";
			throw std::runtime_error{"Card table continuity error"};
		}
	}
	for (const std::pair<wxDataViewItem, Deck *> &pair : deck_rows)
	{
		wxVariant var{};
		browse_decks->GetValue(var, browse_decks->ItemToRow(pair.first), 0);
		//std::cerr << var.GetString().ToStdString() << "\t" << pair.second->canonical() << "\n";
		if (var.GetString().ToStdString() != pair.second->canonical())
		{
			std::cerr << "Deck mapping mismatch:  " << var.GetString().ToStdString() << " vs " << pair.second->canonical() << "\n";
			throw std::runtime_error{"Deck table continuity error"};
		}
	}
}

void MainFrame::populate_decktree(Deck *d)
{
	if (d == &Deck::root)
	{
		tree_decks->DeleteAllItems();
		deckids.clear();
		deckids[""] = tree_decks->AddRoot(_(""));
	}
	for (Set::SetType st : Set::settypes())
	{
		deckids[d->canonical() + ":" + Set::st2str(st)] = tree_decks->AppendItem(deckids[d->canonical()], Set::st2str(st) + " (" + util::t2s<int>(d->set(st).size()) + ")");
		tree_decks->SetItemTextColour(deckids[d->canonical() + ":" + Set::st2str(st)], wxColor("BLUE"));
	}
	std::vector<Deck *> temp{};
	for (Deck *child : d->children()) temp.push_back(child);
	std::sort(temp.begin(), temp.end(), [](Deck *a, Deck *b) { return a->name() < b->name(); });
	for (Deck *child : temp)
	{
		deckids[child->canonical()] = tree_decks->AppendItem(deckids[d->canonical()], child->name() + " (" + util::t2s<int>(child->set(Set::SetType::NORMAL).size()) + ")");
		populate_decktree(child);
	}
}

void MainFrame::populate_cardtable(std::string filter)
{
	browse_cards->DeleteAllItems();
	card_rows.clear();
	for (Card &c : Card::cards()) if (c.match(filter)) table_addcard(c);
}

void MainFrame::populate_decktable()
{
	browse_decks->DeleteAllItems();
	deck_rows.clear();
	for (Deck &d : Deck::decks()) table_adddeck(d);
}

void MainFrame::populate_bankview()
{
	if (curset) bank_view->SetPage(_(curset->deck().bank().htmlview()));
	else bank_view->SetPage(_("<html><body><center>(No deck selected)</center></body></html>"));
}

void MainFrame::refresh_views(int mode) try
{
	if (mode & 0x1) populate_cardtable();
	if (mode & 0x2) populate_decktable();
	if (mode & 0x4) populate_decktree();
	if (mode & 0x8) populate_bankview();
	stattext();
	debug_tablecheck();
}
catch (std::runtime_error &e) { except(e); }

int MainFrame::table_addcard(Card &card)
{
	browse_cards->AppendItem(cols2wxvec(card_columns, card.vectorize(card_columns)));
	int row = browse_cards->GetItemCount() - 1;
	card_rows[browse_cards->RowToItem(row)] = &card;
	return row;
}

void MainFrame::table_delcard(int row)
{
	//wxDataViewItem item = browse_cards->RowToItem(row);
	//std::cerr << ">> " << item.GetID() << "\t" << browse_cards->ItemToRow(item) << "\n";
	card_rows.erase(browse_cards->RowToItem(row));
	browse_cards->DeleteItem(row);
	//std::cerr << ">> " << item.GetID() << "\t" << browse_cards->ItemToRow(item) << "\n";
}

int MainFrame::table_adddeck(Deck& deck)
{
	browse_decks->AppendItem(cols2wxvec(deck_columns, deck.vectorize(deck_columns)));
	int row = browse_decks->GetItemCount() - 1;
	deck_rows[browse_decks->RowToItem(row)] = &deck;
	return row;
}

void MainFrame::table_deldeck(int row) // Recursively deletes the deck, child deck entries and cards as well
{
	wxVariant val{};
	browse_decks->GetValue(val, row, 0);
	for (Card *c : row2deck(row)->cards()) table_delcard(card2row(c));
	for (Deck *child : row2deck(row)->children()) table_deldeck(deck2row(child));
	Deck::del(*row2deck(row));
	deck_rows.erase(browse_decks->RowToItem(row));
	browse_decks->DeleteItem(row);
}

/******************************************************************************
 * Action handlers
 ******************************************************************************/

void MainFrame::quit(wxCommandEvent& event) try
{
	Close();
}
catch(std::exception &e) { except(e); }

void MainFrame::about(wxCommandEvent& event) try
{
	wxMessageBox(_(PROGRAM " " VERSION "\nBy " AUTHOR " " DATE), _("About"), wxOK | wxICON_INFORMATION, this);
}
catch(std::exception &e) { except(e); }

void MainFrame::close(wxCloseEvent &event) try
{
	backend::cleanup();
	wxExit();
}
catch(std::exception &e) { except(e); }

void MainFrame::refresh(wxCommandEvent &event) try
{
	refresh_views();
}
catch(std::exception &e) { except(e); }

void MainFrame::switch_deck(wxTreeEvent& event) try
{
	std::pair<Deck *, Set::SetType> pair = tree2deck(tree_decks->GetSelection());
	curset = &pair.first->set(pair.second);
	refresh_views(0x3);
}
catch(std::exception &e)
{
	err(e.what());
	event.Veto();
}

void MainFrame::activate_deck(wxTreeEvent& event) try
{
	notebook->ChangeSelection(2);
	pagechange();
}
catch(std::exception &e) { except(e); }

void MainFrame::page_changed(wxNotebookEvent &event) try
{
	pagechange();
}
catch(std::exception &e) { except(e); }

void MainFrame::offset_advanced(wxCommandEvent &event) try
{
	Deck::shift_all(-1);
	refresh_views();
}
catch(std::exception &e) { except(e); }

void MainFrame::offset_reversed(wxCommandEvent &event) try
{
	Deck::shift_all(1);
	refresh_views();
}
catch(std::exception &e) { except(e); }

void MainFrame::card_added(wxCommandEvent &event) try
{
	addcard();
}
catch(std::exception &e) { except(e); }

void MainFrame::card_deleted(wxCommandEvent &event) try
{
	std::string curdeck{};
	if (curset) curdeck = curset->deck().canonical();
	int row = browse_cards->GetSelectedRow();
	if (row == wxNOT_FOUND) return;
	delcard(row);
	if (curset && ! Deck::exists(curdeck))
	{
		curset = nullptr;
		stattext();
	}
}
catch(std::exception &e) { except(e); }

void MainFrame::card_searched(wxCommandEvent &event) try
{
	populate_cardtable(event.GetString().ToStdString());
}
catch(std::exception &e) { except(e); }

void MainFrame::card_edited(wxDataViewEvent &event) try
{
	std::string curdeck{};
	assert(notebook->GetSelection() == 3);
	if (curset) curdeck = curset->deck().canonical();
	int row = browse_cards->GetSelectedRow();
	if (row == wxNOT_FOUND) return;
	Deck *deck;
	int offset;
	int delay;
	Card::Status status;
	for (unsigned int col = 0; col < card_columns.size(); col++)
	{
		wxVariant variant{};
		browse_cards->GetValue(variant, row, col);
		std::string field = card_columns[col].title;
		if (field == "Deck") deck = &Deck::get(variant.GetString().ToStdString());
		else if (field == "Status") status = Card::str2stat(variant.GetString().ToStdString());
		else if (field == "Offset") offset = variant.GetInteger(); //nextdue = date{variant.GetDateTime().GetTicks()};
		else if (field == "Interval") delay = variant.GetInteger();
		else if (card_columns[col].type == coldesc::Type::FIELD) row2card(row)->field(field, variant.GetString().ToStdString());
	}
	row2card(row)->edit(*deck, offset, delay, status);
	if (curset && ! Deck::exists(curdeck)) curset = nullptr;
	refresh_views(0x6); // Update deck tree and table
	
}
catch(std::exception &e)
{
	err(e.what());
	event.Veto();
}

void MainFrame::deck_added(wxCommandEvent &event) try
{
	std::string deckname = Deck::freename();
	Deck &deck = Deck::add(deckname); // TODO Defaults
	table_adddeck(deck);
	curset = &deck.set(Set::SetType::NORMAL /*settype*/);
	// TODO Disabling elements based on parameters?
	refresh_views(0x4); // Update deck tree and set current deck to the new one
	browse_decks->SelectRow(deck2row(&deck));
	browse_decks->EnsureVisible(browse_cards->RowToItem(deck2row(&deck)));
}
catch(std::exception &e)
{
	err(e.what());
	refresh_views(0x4);
}

void MainFrame::deck_deleted(wxCommandEvent &event) try
{
	std::string curdeck{};
	if (curset) curdeck = curset->deck().canonical();
	int row = browse_decks->GetSelectedRow();
	if (row == wxNOT_FOUND) return;
	if (curset != nullptr && row2deck(row) == &curset->deck()) curset = nullptr;
	table_deldeck(row); // This also deletes the deck
	if (curset && ! Deck::exists(curdeck)) curset = nullptr;
	refresh_views(0x5); // Update deck tree and card table
}
catch(std::exception &e)
{
	err(e.what());
	refresh_views(0x5);
}

void MainFrame::setbankitem(const std::string word, bool enabled)
{
	if (enabled) curset->deck().bank().enable(word);
	else curset->deck().bank().disable(word);
}

void MainFrame::bankitem_set(wxHtmlLinkEvent &event) try
{
	std::string word = event.GetLinkInfo().GetHref().ToStdString();
	setbankitem(word, ! curset->deck().bank().enabled(word));
}
catch(std::exception &e) { except(e); }

void MainFrame::deck_edited(wxDataViewEvent &event) try
{
	assert(notebook->GetSelection() == 4);
	Deck *deck = deck_rows[event.GetItem()]; //browse_decks->GetSelectedRow();
	//assert(row != wxNOT_FOUND);
	std::string name = "";
	bool explic = false;
	Card::Field front = Card::Field::NONE, back = Card::Field::NONE;
	for (unsigned int col = 0; col < deck_columns.size(); col++)
	{
		wxVariant variant{};
		browse_decks->GetValue(variant, deck2row(deck), col);
		std::string field = deck_columns[col].title;
		if (field == "Name") name = variant.GetString().ToStdString();
		else if (field == "Explicit") explic = variant.GetBool();
		else if (field == "Front") front = Card::str2sit(variant.GetString().ToStdString());
		else if (field == "Back") back = Card::str2sit(variant.GetString().ToStdString());
	}
	if (! deck->edit(name, explic)) throw std::runtime_error{"Deck editing failed"}; // TODO This should not be a fatal error; just pop something up
	// TODO Get selected row
	refresh_views(0x7);
	// TODO Restore selected row
}
catch(std::exception &e)
{
	err(e.what());
	event.Veto();
}

void MainFrame::keydown(wxKeyEvent &event) try
{
	int page = notebook->GetSelection();
	int key = event.GetKeyCode();
	if (page != 2) throw std::runtime_error{"keydown called in wrong tab"};
	switch (key)
	{
		case 'A':
			notebook->ChangeSelection(3);
			pagechange();
			addcard();
			return;
		case 'Q':
			Close(true);
	}
	if (curset->size() == 0) return;
	Card::UpdateType ut;
	Card &curcard = curset->top();
	switch (key)
	{
		case WXK_NUMPAD_ENTER:
			if (disp == Set::DispType::BACK || disp == Set::DispType::HINT) disp = Set::DispType::FRONT;
			else disp = Set::DispType::BACK;
			showcard();
			return;
		case WXK_NUMPAD1:
			ut = Card::UpdateType::DECR;
			break;
		case WXK_NUMPAD2:
			ut = Card::UpdateType::NORM;
			break;
		case WXK_NUMPAD3:
			ut = Card::UpdateType::INCR;
			break;
		case WXK_NUMPAD4:
			ut = Card::UpdateType::RESET;
			break;
		case WXK_NUMPAD5:
			ut = Card::UpdateType::SUSP;
			break;
		case WXK_NUMPAD6:
			ut = Card::UpdateType::DONE;
			break;
		case WXK_NUMPAD9:
			ut = Card::UpdateType::BURY;
			break;
		case 'H':
			if (disp == Set::DispType::HINT) disp = Set::DispType::FRONT;
			else disp = Set::DispType::HINT;
			showcard();
			return;
		case 'E':
			notebook->ChangeSelection(3);
			pagechange();
			browse_cards->SelectRow(card2row(&curcard));
			browse_cards->EnsureVisible(browse_cards->RowToItem(card2row(&curcard)));
			return;
		case 'D':
			delcard(card2row(&curcard));
			return;
		case 'S':
			curset->shuffle();
			showcard();
			return;
		default:
			return;
	}
	disp = Set::DispType::FRONT;
	curset->update(ut);
	// TODO Update card and deck tables to reflect updated information
	//table_updaterow(browse_cards, card2row(&curcard), cols2wxvec(card_columns, curcard.vectorize(card_columns)));
	//table_updaterow(browse_decks, 0 /* FIXME */, cols2wxvec(deck_columns, curdeck->vectorize(deck_columns));
	showcard();
}
catch(std::exception &e) { except(e); }

void MainFrame::key_view(wxKeyEvent &event) try
{
	int key = event.GetKeyCode();
	switch(key)
	{
		case WXK_TAB:
			std::cout << "tab\n";
			break;
		case WXK_RETURN:
			std::cout << "enter\n";
			break;
		default:
			return;
	}
}
catch(std::exception &e) { except(e); }
