Tango
=====

All right, I've done it.  In a classic example of NIH syndrome, I've invested a disproportionate amount of time in completely reimplementing [Anki](http://ankisrs.net/) to create a stripped-down, buggy, inflexible version that slightly better fits my workflow, stomping on the very philosophy that makes Anki so great in the process.  This is Tango, a flash-card study system designed to help memorize vocabulary.  The name comes from the Japanese word for "vocabulary" (creative, I know), and the program is strongly slanted toward studying Japanese, but I hope to make it more general in the near future.

Tango is very much in its early stages of development.  It is functional enough to meet most of my needs, but far too much is still hardcoded into the program or done manually by me.  Expect this to change as the program matures.

Usage
-----

Compilation should be relatively simple.  Just run `make` in the root directory, and the executable should be placed in the `build` subdirectory.  I'm using a NetBeans-generated Makefile, and I'm not sure about how that works internally, so if you try to compile it and it doesn't work, let me know because I'll never figure it out myself.

Tango uses a SQLite3 database at the user's `XDG_CONFIG_HOME/tango/decks.db` for storing configuration and practice data.  At the moment, the program lacks routines for setting up this database the first time it is opened, which renders it pretty much useless for new users.  This functionality should be coming soon, but at the moment development is driven much more by my needs than by community interests.  If you find yourself itching to try Tango out yourself, shoot me a message and I'll reprioritize my tasks.

Decks, Cards, Sets
------------------

Each item for study in Tango is a *card*.  Unlike a real flashcard, a Tango card can have more than two "sides", or fields.  Currently, the fields are *temporarily* hardcoded into the program as "Expression", "Reading", and "Meaning".  Which fields to display are selected according to "sets", explained momentarily.

Card are organized into *decks* based on the user's wishes; generally, cards in the same deck should be cards that are thematically related to each other or cards that should be studied together.  Decks can be either *explicit* or *implicit*.  Explicit decks are created explicitly by the user using the "Add" button in the decks panel and can have settings associated with them.  These settings are stored in the database and persist between sessions.  Implicit decks are not stored in the database, but exist simply by virtue of the fact that there is a card that thinks it is in that deck.  If all cards in an implicit deck are deleted or moved elsewhere, the deck ceases to exist.  Implicit decks inherit their settings from their parent decks.  There is a special deck, the root deck, that is explicit and undeletable and uses settings that are *temporarily* hardcoded into the project but will eventually be settable through the preferences window.

How does a card "think" that it is in a deck?  In Tango, cards and decks are only loosely coupled.  A list of explicit decks is stored in the database, and a "path" is stored for each card.  This path is a slash-delimited string describing where the card is.  For example, I may have two explicit decks in my database, called "1" and "1/2", as well as cards with paths "1", "1/2", "1/3", and "4".  In this case, "1" is a top-level explicit deck, and "1/2" is an explicit subdeck of "1".  "1" also has an implicit subdeck, "1/3", that exists because there is a card in it.  There is also an implicit top-level deck called "4".  If the card in "4" is deleted, the deck will automatically disappear.  "1" and "1/2" have their own settings, but "1/3" inherits its settings from "1" and "4" inherits its settings from the root deck.

Each deck has several *sets*.  A set is a collection of cards to be practiced together in a certain way.  Each set is associated with a subset of the cards in the deck, as well as which fields of those cards to show as the "question" and "answer" of the card.  An example should clarify this.  When studying Japanese, I know the kanji (Chinese characters) for some of the words, but not for others.  Therefore, for the cards whose kanji I know, I want to be shown the English meaning of the word and write down the kanji, then be shown the kanji so I can check whether I was correct.  For other cards, I want to be shown either the Japanese reading or the English meaning, guess the other, and then be shown the other so I can check myself.  Therefore, I will have a set called "Kanji", which only contains cards whose kanji I know, whose "front" is the meaning and whose "back" is the kanji; and a set called "Kana", which contains all the other cards, whose front is either the meaning or reading and whose back is the other.  At the moment, the set setup is *temporarily* hardcoded in the sort of configuration I have just described, only a little more complicated; all decks have four sets ("Normal", "All", "Kanji", and "Kana"), and each one  has rules for which fields to display for the "front", the "back", and a "hint".  At the moment, this is pretty easy to use since it is completely unconfigurable; eventually I'll make it configurable and the whole thing will get more complicated to use.

Sets also do not exist in the database; they are generated by the program when it starts up based on which cards are "due" for studying.  A card can be in multiple sets -- indeed, the "All" set contains all cards in the deck, in case you want a thorough go-over.  Cards are determined to be "due" based on how long it's been since you've studied them, but unlike Anki, Tango does not work in real-time.  When you are ready to study more cards, click the right arrow button at the bottom of the decks pane to advance every card in the system, and the next set of "due" cards will be presented.  You can also go back in time by clicking the left arrow.  I plan to have an option eventually to automatically advance the cards every 24 hours, which will emulate Anki and restore some of the time pressure that pushes you to study every day.  

I mentioned earlier that I only want words with kani I know to appear in the "Kanji" set.  To allow the program to know which kanji I know, each explicit deck is associated with a "bank" of kanji.  This is one of the rather Japanese-specific aspects of Tango, and I'm not sure yet how I'll generalize it to other languages and concepts.  The bank interface is still rather primitive and incomplete, but more should be coming after I get some of the more major issues out of the way.

Studying
--------

Okay, how do you actually use Tango to study?  Assuming you have a populated database with some decks and cards (which, as I said, will come soon), select the set you want to study within the deck you want to study (or select the deck name, in which case the "Normal" set will be selected), and switch to the study pane.  The bottom status bar will display, from left to right, the current set, the number of cards to be studied in that set, and the current study interval on the current card.  Use the following keys to interact with the program:

  - Numpad `Enter`: Toggle displaying the "front" and "back" of the current card

  - Numpad `1`: Update the card with half of the current study interval.  That means that the current study interval displayed in the status bar will be halved and the the card's due date will be updated to be that many steps in the future.  If the current study interval is 1 or 0, it will be updated to 0 and placed at the back of the set to be studied again this session.  Do this if you got the answer wrong or felt uncertain about your answer.

  - Numpad `2`: Update the card with the current study interval.  Do this if you got the answer right and feel moderately confident about it.

  - Numpad `3`: Update the card with twice the current study interval.  Do this if you got the answer right and feel very confident.  If the new study interval exceeds 365 days (this will eventually be configurable), the card will be marked "done" and you will not need to study it again.

  - Numpad `4`: Reset the card.  This sets its study interval to 0, marking it for immediate re-studying, and places it at the back of the current set.  Do this if you think the card needs to be studied again right away.

  - Numpad `5`: Suspend the card.  It will not be presented for study and the time until its next study will not be decremented until you un-suspend it through the cards pane.

  - Numpad `6`: Immediately mark the card "done".  It will not be presented for study any more.  There is currently no practical difference between marking a card "suspended" and "done"; it's merely for book-keeping.

  - Numpad `9`: Bury the card.  Move the card to the back of the set without updating it at all.  This allows you to "put off" studying a card.  This and all of the numpad number keys above "advance" the deck to the next card.

  - `H`: Toggle displaying the card hint.  Which field the hint is depends on the display settings.

  - `E`: Jump to the current card in the cards pane, allowing you to edit its fields or other properties.

  - `D`: Delete the current card from its deck and from the database.

  - `S`: Shuffle the set.

Thus, the standard study session consists of selecting a set to study, and then repeatedly looking at a card, thinking of the correct answer, pressing `Enter` to check your answer, and pressing a number key based on how good your answer was.  I promise that I'll add some new keys soon so that poor users without numpads can still use this program.  The new status of each card is saved immediately in the database after it is studied or edited, so there is no need to manually save.

More documentation will come as I have time to flesh out the program itself.

