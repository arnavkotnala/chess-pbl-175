#pragma once

struct TriviaQuestionData {
    const char* question;
    const char* options[4];
    int correct;
};

static const TriviaQuestionData g_triviaBank[100] = {
    {
        "How many squares are on a standard chessboard?",
        { "16", "20", "64", "8", },
        2
    },
    {
        "What is it called when a pawn moves two squares and can be captured as if it only moved one?",
        { "Checkmate", "En passant", "Castling", "Fork", },
        1
    },
    {
        "Which piece moves in an L-shape?",
        { "Horse", "Knight", "Cavalry", "Joker", },
        1
    },
    {
        "Which piece can move any number of squares in any direction?",
        { "Queen", "King", "Rook", "Bishop", },
        0
    },
    {
        "What is the most popular chess opening at grandmaster level?",
        { "Ruy Lopez", "Sicilian Defense", "Kings Indian", "Dutch Defense", },
        1
    },
    {
        "Who was the first undisputed World Chess Champion from the USA?",
        { "Garry Kasparov", "Magnus Carlsen", "Bobby Fischer", "Anatoly Karpov", },
        2
    },
    {
        "What is it called when the king and rook swap positions?",
        { "En passant", "Promotion", "Castling", "Check", },
        2
    },
    {
        "How many points is a bishop worth in standard chess valuation?",
        { "3", "2", "1", "4", },
        0
    },
    {
        "Which nationality is Garry Kasparov?",
        { "Norwegian", "Russian", "American", "Cuban", },
        1
    },
    {
        "The Caro-Kann generally starts with which move?",
        { "e4", "d4", "c4", "Nf3", },
        0
    },
    {
        "What does the term 'Discovered Attack' refer to in chess?",
        { "A tactical motif.", "An opening variation.", "An endgame position.", "A type of draw.", },
        0
    },
    {
        "Which of the following is true about the King?",
        { "It moves diagonally.", "It can jump over other pieces.", "It is a minor piece.", "It captures the way it moves.", },
        3
    },
    {
        "Which nationality is Jose Raul Capablanca?",
        { "Norwegian", "Russian", "American", "Cuban", },
        1
    },
    {
        "The King's Indian Defense generally starts with which move?",
        { "e4", "d4", "c4", "Nf3", },
        0
    },
    {
        "What does the term 'En Passant' refer to in chess?",
        { "A tactical motif.", "An opening variation.", "An endgame position.", "A type of draw.", },
        0
    },
    {
        "Which of the following is true about the Rook?",
        { "It moves diagonally.", "It can jump over other pieces.", "It is a minor piece.", "It captures the way it moves.", },
        3
    },
    {
        "Which nationality is Anatoly Karpov?",
        { "Norwegian", "Russian", "American", "Cuban", },
        1
    },
    {
        "The Ruy Lopez generally starts with which move?",
        { "e4", "d4", "c4", "Nf3", },
        0
    },
    {
        "What does the term 'Discovered Attack' refer to in chess?",
        { "A tactical motif.", "An opening variation.", "An endgame position.", "A type of draw.", },
        0
    },
    {
        "Which of the following is true about the Knight?",
        { "It moves diagonally.", "It can jump over other pieces.", "It is a minor piece.", "It captures the way it moves.", },
        3
    },
    {
        "Which nationality is Alexander Alekhine?",
        { "Norwegian", "Russian", "American", "Cuban", },
        1
    },
    {
        "The Sicilian Defense generally starts with which move?",
        { "e4", "d4", "c4", "Nf3", },
        0
    },
    {
        "What does the term 'En Passant' refer to in chess?",
        { "A tactical motif.", "An opening variation.", "An endgame position.", "A type of draw.", },
        0
    },
    {
        "Which of the following is true about the King?",
        { "It moves diagonally.", "It can jump over other pieces.", "It is a minor piece.", "It captures the way it moves.", },
        3
    },
    {
        "Which nationality is Bobby Fischer?",
        { "Norwegian", "Russian", "American", "Cuban", },
        1
    },
    {
        "The Italian Game generally starts with which move?",
        { "e4", "d4", "c4", "Nf3", },
        0
    },
    {
        "What does the term 'Discovered Attack' refer to in chess?",
        { "A tactical motif.", "An opening variation.", "An endgame position.", "A type of draw.", },
        0
    },
    {
        "Which of the following is true about the Rook?",
        { "It moves diagonally.", "It can jump over other pieces.", "It is a minor piece.", "It captures the way it moves.", },
        3
    },
    {
        "Which nationality is Magnus Carlsen?",
        { "Norwegian", "Russian", "American", "Cuban", },
        1
    },
    {
        "The French Defense generally starts with which move?",
        { "e4", "d4", "c4", "Nf3", },
        0
    },
    {
        "What does the term 'En Passant' refer to in chess?",
        { "A tactical motif.", "An opening variation.", "An endgame position.", "A type of draw.", },
        0
    },
    {
        "Which of the following is true about the Knight?",
        { "It moves diagonally.", "It can jump over other pieces.", "It is a minor piece.", "It captures the way it moves.", },
        3
    },
    {
        "Which nationality is Mikhail Tal?",
        { "Norwegian", "Russian", "American", "Cuban", },
        1
    },
    {
        "The Queen's Gambit generally starts with which move?",
        { "e4", "d4", "c4", "Nf3", },
        0
    },
    {
        "What does the term 'Discovered Attack' refer to in chess?",
        { "A tactical motif.", "An opening variation.", "An endgame position.", "A type of draw.", },
        0
    },
    {
        "Which of the following is true about the King?",
        { "It moves diagonally.", "It can jump over other pieces.", "It is a minor piece.", "It captures the way it moves.", },
        3
    },
    {
        "Which nationality is Garry Kasparov?",
        { "Norwegian", "Russian", "American", "Cuban", },
        1
    },
    {
        "The Caro-Kann generally starts with which move?",
        { "e4", "d4", "c4", "Nf3", },
        0
    },
    {
        "What does the term 'En Passant' refer to in chess?",
        { "A tactical motif.", "An opening variation.", "An endgame position.", "A type of draw.", },
        0
    },
    {
        "Which of the following is true about the Rook?",
        { "It moves diagonally.", "It can jump over other pieces.", "It is a minor piece.", "It captures the way it moves.", },
        3
    },
    {
        "Which nationality is Jose Raul Capablanca?",
        { "Norwegian", "Russian", "American", "Cuban", },
        1
    },
    {
        "The King's Indian Defense generally starts with which move?",
        { "e4", "d4", "c4", "Nf3", },
        0
    },
    {
        "What does the term 'Discovered Attack' refer to in chess?",
        { "A tactical motif.", "An opening variation.", "An endgame position.", "A type of draw.", },
        0
    },
    {
        "Which of the following is true about the Knight?",
        { "It moves diagonally.", "It can jump over other pieces.", "It is a minor piece.", "It captures the way it moves.", },
        3
    },
    {
        "Which nationality is Anatoly Karpov?",
        { "Norwegian", "Russian", "American", "Cuban", },
        1
    },
    {
        "The Ruy Lopez generally starts with which move?",
        { "e4", "d4", "c4", "Nf3", },
        0
    },
    {
        "What does the term 'En Passant' refer to in chess?",
        { "A tactical motif.", "An opening variation.", "An endgame position.", "A type of draw.", },
        0
    },
    {
        "Which of the following is true about the King?",
        { "It moves diagonally.", "It can jump over other pieces.", "It is a minor piece.", "It captures the way it moves.", },
        3
    },
    {
        "Which nationality is Alexander Alekhine?",
        { "Norwegian", "Russian", "American", "Cuban", },
        1
    },
    {
        "The Sicilian Defense generally starts with which move?",
        { "e4", "d4", "c4", "Nf3", },
        0
    },
    {
        "What does the term 'Discovered Attack' refer to in chess?",
        { "A tactical motif.", "An opening variation.", "An endgame position.", "A type of draw.", },
        0
    },
    {
        "Which of the following is true about the Rook?",
        { "It moves diagonally.", "It can jump over other pieces.", "It is a minor piece.", "It captures the way it moves.", },
        3
    },
    {
        "Which nationality is Bobby Fischer?",
        { "Norwegian", "Russian", "American", "Cuban", },
        1
    },
    {
        "The Italian Game generally starts with which move?",
        { "e4", "d4", "c4", "Nf3", },
        0
    },
    {
        "What does the term 'En Passant' refer to in chess?",
        { "A tactical motif.", "An opening variation.", "An endgame position.", "A type of draw.", },
        0
    },
    {
        "Which of the following is true about the Knight?",
        { "It moves diagonally.", "It can jump over other pieces.", "It is a minor piece.", "It captures the way it moves.", },
        3
    },
    {
        "Which nationality is Magnus Carlsen?",
        { "Norwegian", "Russian", "American", "Cuban", },
        1
    },
    {
        "The French Defense generally starts with which move?",
        { "e4", "d4", "c4", "Nf3", },
        0
    },
    {
        "What does the term 'Discovered Attack' refer to in chess?",
        { "A tactical motif.", "An opening variation.", "An endgame position.", "A type of draw.", },
        0
    },
    {
        "Which of the following is true about the King?",
        { "It moves diagonally.", "It can jump over other pieces.", "It is a minor piece.", "It captures the way it moves.", },
        3
    },
    {
        "Which nationality is Mikhail Tal?",
        { "Norwegian", "Russian", "American", "Cuban", },
        1
    },
    {
        "The Queen's Gambit generally starts with which move?",
        { "e4", "d4", "c4", "Nf3", },
        0
    },
    {
        "What does the term 'En Passant' refer to in chess?",
        { "A tactical motif.", "An opening variation.", "An endgame position.", "A type of draw.", },
        0
    },
    {
        "Which of the following is true about the Rook?",
        { "It moves diagonally.", "It can jump over other pieces.", "It is a minor piece.", "It captures the way it moves.", },
        3
    },
    {
        "Which nationality is Garry Kasparov?",
        { "Norwegian", "Russian", "American", "Cuban", },
        1
    },
    {
        "The Caro-Kann generally starts with which move?",
        { "e4", "d4", "c4", "Nf3", },
        0
    },
    {
        "What does the term 'Discovered Attack' refer to in chess?",
        { "A tactical motif.", "An opening variation.", "An endgame position.", "A type of draw.", },
        0
    },
    {
        "Which of the following is true about the Knight?",
        { "It moves diagonally.", "It can jump over other pieces.", "It is a minor piece.", "It captures the way it moves.", },
        3
    },
    {
        "Which nationality is Jose Raul Capablanca?",
        { "Norwegian", "Russian", "American", "Cuban", },
        1
    },
    {
        "The King's Indian Defense generally starts with which move?",
        { "e4", "d4", "c4", "Nf3", },
        0
    },
    {
        "What does the term 'En Passant' refer to in chess?",
        { "A tactical motif.", "An opening variation.", "An endgame position.", "A type of draw.", },
        0
    },
    {
        "Which of the following is true about the King?",
        { "It moves diagonally.", "It can jump over other pieces.", "It is a minor piece.", "It captures the way it moves.", },
        3
    },
    {
        "Which nationality is Anatoly Karpov?",
        { "Norwegian", "Russian", "American", "Cuban", },
        1
    },
    {
        "The Ruy Lopez generally starts with which move?",
        { "e4", "d4", "c4", "Nf3", },
        0
    },
    {
        "What does the term 'Discovered Attack' refer to in chess?",
        { "A tactical motif.", "An opening variation.", "An endgame position.", "A type of draw.", },
        0
    },
    {
        "Which of the following is true about the Rook?",
        { "It moves diagonally.", "It can jump over other pieces.", "It is a minor piece.", "It captures the way it moves.", },
        3
    },
    {
        "Which nationality is Alexander Alekhine?",
        { "Norwegian", "Russian", "American", "Cuban", },
        1
    },
    {
        "The Sicilian Defense generally starts with which move?",
        { "e4", "d4", "c4", "Nf3", },
        0
    },
    {
        "What does the term 'En Passant' refer to in chess?",
        { "A tactical motif.", "An opening variation.", "An endgame position.", "A type of draw.", },
        0
    },
    {
        "Which of the following is true about the Knight?",
        { "It moves diagonally.", "It can jump over other pieces.", "It is a minor piece.", "It captures the way it moves.", },
        3
    },
    {
        "Which nationality is Bobby Fischer?",
        { "Norwegian", "Russian", "American", "Cuban", },
        1
    },
    {
        "The Italian Game generally starts with which move?",
        { "e4", "d4", "c4", "Nf3", },
        0
    },
    {
        "What does the term 'Discovered Attack' refer to in chess?",
        { "A tactical motif.", "An opening variation.", "An endgame position.", "A type of draw.", },
        0
    },
    {
        "Which of the following is true about the King?",
        { "It moves diagonally.", "It can jump over other pieces.", "It is a minor piece.", "It captures the way it moves.", },
        3
    },
    {
        "Which nationality is Magnus Carlsen?",
        { "Norwegian", "Russian", "American", "Cuban", },
        1
    },
    {
        "The French Defense generally starts with which move?",
        { "e4", "d4", "c4", "Nf3", },
        0
    },
    {
        "What does the term 'En Passant' refer to in chess?",
        { "A tactical motif.", "An opening variation.", "An endgame position.", "A type of draw.", },
        0
    },
    {
        "Which of the following is true about the Rook?",
        { "It moves diagonally.", "It can jump over other pieces.", "It is a minor piece.", "It captures the way it moves.", },
        3
    },
    {
        "Which nationality is Mikhail Tal?",
        { "Norwegian", "Russian", "American", "Cuban", },
        1
    },
    {
        "The Queen's Gambit generally starts with which move?",
        { "e4", "d4", "c4", "Nf3", },
        0
    },
    {
        "What does the term 'Discovered Attack' refer to in chess?",
        { "A tactical motif.", "An opening variation.", "An endgame position.", "A type of draw.", },
        0
    },
    {
        "Which of the following is true about the Knight?",
        { "It moves diagonally.", "It can jump over other pieces.", "It is a minor piece.", "It captures the way it moves.", },
        3
    },
    {
        "Which nationality is Garry Kasparov?",
        { "Norwegian", "Russian", "American", "Cuban", },
        1
    },
    {
        "The Caro-Kann generally starts with which move?",
        { "e4", "d4", "c4", "Nf3", },
        0
    },
    {
        "What does the term 'En Passant' refer to in chess?",
        { "A tactical motif.", "An opening variation.", "An endgame position.", "A type of draw.", },
        0
    },
    {
        "Which of the following is true about the King?",
        { "It moves diagonally.", "It can jump over other pieces.", "It is a minor piece.", "It captures the way it moves.", },
        3
    },
    {
        "Which nationality is Jose Raul Capablanca?",
        { "Norwegian", "Russian", "American", "Cuban", },
        1
    },
    {
        "The King's Indian Defense generally starts with which move?",
        { "e4", "d4", "c4", "Nf3", },
        0
    },
    {
        "What does the term 'Discovered Attack' refer to in chess?",
        { "A tactical motif.", "An opening variation.", "An endgame position.", "A type of draw.", },
        0
    },
    {
        "Which of the following is true about the Rook?",
        { "It moves diagonally.", "It can jump over other pieces.", "It is a minor piece.", "It captures the way it moves.", },
        3
    },
};
