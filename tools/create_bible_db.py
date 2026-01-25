#!/usr/bin/env python3
"""
Bible Database Creator for Clarity

Creates an SQLite database with Bible verses for use with Clarity's
scripture lookup feature.

Usage:
    python create_bible_db.py [output_path]

    output_path: Path for the output database (default: bible.db)

The script creates a database with:
- books table: Bible book information
- verses table: Individual verses with text
- verses_fts: Full-text search virtual table

Initial data includes a sample of KJV verses for testing.
For full Bible data, extend the SAMPLE_VERSES dictionary or
import from a public domain Bible text file.
"""

import sqlite3
import sys
import os

# Book order and information
BOOKS = [
    # Old Testament
    ("Genesis", "Gen", "OT", 50),
    ("Exodus", "Exod", "OT", 40),
    ("Leviticus", "Lev", "OT", 27),
    ("Numbers", "Num", "OT", 36),
    ("Deuteronomy", "Deut", "OT", 34),
    ("Joshua", "Josh", "OT", 24),
    ("Judges", "Judg", "OT", 21),
    ("Ruth", "Ruth", "OT", 4),
    ("1 Samuel", "1Sam", "OT", 31),
    ("2 Samuel", "2Sam", "OT", 24),
    ("1 Kings", "1Kgs", "OT", 22),
    ("2 Kings", "2Kgs", "OT", 25),
    ("1 Chronicles", "1Chr", "OT", 29),
    ("2 Chronicles", "2Chr", "OT", 36),
    ("Ezra", "Ezra", "OT", 10),
    ("Nehemiah", "Neh", "OT", 13),
    ("Esther", "Esth", "OT", 10),
    ("Job", "Job", "OT", 42),
    ("Psalms", "Ps", "OT", 150),
    ("Proverbs", "Prov", "OT", 31),
    ("Ecclesiastes", "Eccl", "OT", 12),
    ("Song of Solomon", "Song", "OT", 8),
    ("Isaiah", "Isa", "OT", 66),
    ("Jeremiah", "Jer", "OT", 52),
    ("Lamentations", "Lam", "OT", 5),
    ("Ezekiel", "Ezek", "OT", 48),
    ("Daniel", "Dan", "OT", 12),
    ("Hosea", "Hos", "OT", 14),
    ("Joel", "Joel", "OT", 3),
    ("Amos", "Amos", "OT", 9),
    ("Obadiah", "Obad", "OT", 1),
    ("Jonah", "Jonah", "OT", 4),
    ("Micah", "Mic", "OT", 7),
    ("Nahum", "Nah", "OT", 3),
    ("Habakkuk", "Hab", "OT", 3),
    ("Zephaniah", "Zeph", "OT", 3),
    ("Haggai", "Hag", "OT", 2),
    ("Zechariah", "Zech", "OT", 14),
    ("Malachi", "Mal", "OT", 4),
    # New Testament
    ("Matthew", "Matt", "NT", 28),
    ("Mark", "Mark", "NT", 16),
    ("Luke", "Luke", "NT", 24),
    ("John", "John", "NT", 21),
    ("Acts", "Acts", "NT", 28),
    ("Romans", "Rom", "NT", 16),
    ("1 Corinthians", "1Cor", "NT", 16),
    ("2 Corinthians", "2Cor", "NT", 13),
    ("Galatians", "Gal", "NT", 6),
    ("Ephesians", "Eph", "NT", 6),
    ("Philippians", "Phil", "NT", 4),
    ("Colossians", "Col", "NT", 4),
    ("1 Thessalonians", "1Thess", "NT", 5),
    ("2 Thessalonians", "2Thess", "NT", 3),
    ("1 Timothy", "1Tim", "NT", 6),
    ("2 Timothy", "2Tim", "NT", 4),
    ("Titus", "Titus", "NT", 3),
    ("Philemon", "Phlm", "NT", 1),
    ("Hebrews", "Heb", "NT", 13),
    ("James", "Jas", "NT", 5),
    ("1 Peter", "1Pet", "NT", 5),
    ("2 Peter", "2Pet", "NT", 3),
    ("1 John", "1John", "NT", 5),
    ("2 John", "2John", "NT", 1),
    ("3 John", "3John", "NT", 1),
    ("Jude", "Jude", "NT", 1),
    ("Revelation", "Rev", "NT", 22),
]

# Sample verses (KJV) for testing
# Format: { "Book": { chapter: { verse: "text", ... }, ... }, ... }
SAMPLE_VERSES = {
    "Genesis": {
        1: {
            1: "In the beginning God created the heaven and the earth.",
            2: "And the earth was without form, and void; and darkness was upon the face of the deep. And the Spirit of God moved upon the face of the waters.",
            3: "And God said, Let there be light: and there was light.",
            4: "And God saw the light, that it was good: and God divided the light from the darkness.",
            5: "And God called the light Day, and the darkness he called Night. And the evening and the morning were the first day.",
            26: "And God said, Let us make man in our image, after our likeness: and let them have dominion over the fish of the sea, and over the fowl of the air, and over the cattle, and over all the earth, and over every creeping thing that creepeth upon the earth.",
            27: "So God created man in his own image, in the image of God created he him; male and female created he them.",
            28: "And God blessed them, and God said unto them, Be fruitful, and multiply, and replenish the earth, and subdue it: and have dominion over the fish of the sea, and over the fowl of the air, and over every living thing that moveth upon the earth.",
        },
    },
    "Psalms": {
        23: {
            1: "The LORD is my shepherd; I shall not want.",
            2: "He maketh me to lie down in green pastures: he leadeth me beside the still waters.",
            3: "He restoreth my soul: he leadeth me in the paths of righteousness for his name's sake.",
            4: "Yea, though I walk through the valley of the shadow of death, I will fear no evil: for thou art with me; thy rod and thy staff they comfort me.",
            5: "Thou preparest a table before me in the presence of mine enemies: thou anointest my head with oil; my cup runneth over.",
            6: "Surely goodness and mercy shall follow me all the days of my life: and I will dwell in the house of the LORD for ever.",
        },
        100: {
            1: "Make a joyful noise unto the LORD, all ye lands.",
            2: "Serve the LORD with gladness: come before his presence with singing.",
            3: "Know ye that the LORD he is God: it is he that hath made us, and not we ourselves; we are his people, and the sheep of his pasture.",
            4: "Enter into his gates with thanksgiving, and into his courts with praise: be thankful unto him, and bless his name.",
            5: "For the LORD is good; his mercy is everlasting; and his truth endureth to all generations.",
        },
        119: {
            105: "Thy word is a lamp unto my feet, and a light unto my path.",
            11: "Thy word have I hid in mine heart, that I might not sin against thee.",
        },
    },
    "Proverbs": {
        3: {
            5: "Trust in the LORD with all thine heart; and lean not unto thine own understanding.",
            6: "In all thy ways acknowledge him, and he shall direct thy paths.",
        },
        22: {
            6: "Train up a child in the way he should go: and when he is old, he will not depart from it.",
        },
    },
    "Isaiah": {
        40: {
            31: "But they that wait upon the LORD shall renew their strength; they shall mount up with wings as eagles; they shall run, and not be weary; and they shall walk, and not faint.",
        },
        41: {
            10: "Fear thou not; for I am with thee: be not dismayed; for I am thy God: I will strengthen thee; yea, I will help thee; yea, I will uphold thee with the right hand of my righteousness.",
        },
        53: {
            5: "But he was wounded for our transgressions, he was bruised for our iniquities: the chastisement of our peace was upon him; and with his stripes we are healed.",
            6: "All we like sheep have gone astray; we have turned every one to his own way; and the LORD hath laid on him the iniquity of us all.",
        },
    },
    "Jeremiah": {
        29: {
            11: "For I know the thoughts that I think toward you, saith the LORD, thoughts of peace, and not of evil, to give you an expected end.",
        },
    },
    "Matthew": {
        5: {
            3: "Blessed are the poor in spirit: for theirs is the kingdom of heaven.",
            4: "Blessed are they that mourn: for they shall be comforted.",
            5: "Blessed are the meek: for they shall inherit the earth.",
            6: "Blessed are they which do hunger and thirst after righteousness: for they shall be filled.",
            7: "Blessed are the merciful: for they shall obtain mercy.",
            8: "Blessed are the pure in heart: for they shall see God.",
            9: "Blessed are the peacemakers: for they shall be called the children of God.",
            14: "Ye are the light of the world. A city that is set on an hill cannot be hid.",
            16: "Let your light so shine before men, that they may see your good works, and glorify your Father which is in heaven.",
        },
        6: {
            9: "After this manner therefore pray ye: Our Father which art in heaven, Hallowed be thy name.",
            10: "Thy kingdom come. Thy will be done in earth, as it is in heaven.",
            11: "Give us this day our daily bread.",
            12: "And forgive us our debts, as we forgive our debtors.",
            13: "And lead us not into temptation, but deliver us from evil: For thine is the kingdom, and the power, and the glory, for ever. Amen.",
            33: "But seek ye first the kingdom of God, and his righteousness; and all these things shall be added unto you.",
        },
        11: {
            28: "Come unto me, all ye that labour and are heavy laden, and I will give you rest.",
            29: "Take my yoke upon you, and learn of me; for I am meek and lowly in heart: and ye shall find rest unto your souls.",
            30: "For my yoke is easy, and my burden is light.",
        },
        28: {
            19: "Go ye therefore, and teach all nations, baptizing them in the name of the Father, and of the Son, and of the Holy Ghost:",
            20: "Teaching them to observe all things whatsoever I have commanded you: and, lo, I am with you alway, even unto the end of the world. Amen.",
        },
    },
    "John": {
        1: {
            1: "In the beginning was the Word, and the Word was with God, and the Word was God.",
            2: "The same was in the beginning with God.",
            3: "All things were made by him; and without him was not any thing made that was made.",
            4: "In him was life; and the life was the light of men.",
            5: "And the light shineth in darkness; and the darkness comprehended it not.",
            14: "And the Word was made flesh, and dwelt among us, (and we beheld his glory, the glory as of the only begotten of the Father,) full of grace and truth.",
        },
        3: {
            16: "For God so loved the world, that he gave his only begotten Son, that whosoever believeth in him should not perish, but have everlasting life.",
            17: "For God sent not his Son into the world to condemn the world; but that the world through him might be saved.",
        },
        10: {
            10: "The thief cometh not, but for to steal, and to kill, and to destroy: I am come that they might have life, and that they might have it more abundantly.",
        },
        14: {
            6: "Jesus saith unto him, I am the way, the truth, and the life: no man cometh unto the Father, but by me.",
            27: "Peace I leave with you, my peace I give unto you: not as the world giveth, give I unto you. Let not your heart be troubled, neither let it be afraid.",
        },
    },
    "Romans": {
        3: {
            23: "For all have sinned, and come short of the glory of God;",
        },
        5: {
            8: "But God commendeth his love toward us, in that, while we were yet sinners, Christ died for us.",
        },
        6: {
            23: "For the wages of sin is death; but the gift of God is eternal life through Jesus Christ our Lord.",
        },
        8: {
            1: "There is therefore now no condemnation to them which are in Christ Jesus, who walk not after the flesh, but after the Spirit.",
            28: "And we know that all things work together for good to them that love God, to them who are the called according to his purpose.",
            31: "What shall we then say to these things? If God be for us, who can be against us?",
            38: "For I am persuaded, that neither death, nor life, nor angels, nor principalities, nor powers, nor things present, nor things to come,",
            39: "Nor height, nor depth, nor any other creature, shall be able to separate us from the love of God, which is in Christ Jesus our Lord.",
        },
        10: {
            9: "That if thou shalt confess with thy mouth the Lord Jesus, and shalt believe in thine heart that God hath raised him from the dead, thou shalt be saved.",
            10: "For with the heart man believeth unto righteousness; and with the mouth confession is made unto salvation.",
        },
        12: {
            1: "I beseech you therefore, brethren, by the mercies of God, that ye present your bodies a living sacrifice, holy, acceptable unto God, which is your reasonable service.",
            2: "And be not conformed to this world: but be ye transformed by the renewing of your mind, that ye may prove what is that good, and acceptable, and perfect, will of God.",
        },
    },
    "1 Corinthians": {
        10: {
            13: "There hath no temptation taken you but such as is common to man: but God is faithful, who will not suffer you to be tempted above that ye are able; but will with the temptation also make a way to escape, that ye may be able to bear it.",
        },
        13: {
            4: "Charity suffereth long, and is kind; charity envieth not; charity vaunteth not itself, is not puffed up,",
            5: "Doth not behave itself unseemly, seeketh not her own, is not easily provoked, thinketh no evil;",
            6: "Rejoiceth not in iniquity, but rejoiceth in the truth;",
            7: "Beareth all things, believeth all things, hopeth all things, endureth all things.",
            8: "Charity never faileth: but whether there be prophecies, they shall fail; whether there be tongues, they shall cease; whether there be knowledge, it shall vanish away.",
            13: "And now abideth faith, hope, charity, these three; but the greatest of these is charity.",
        },
    },
    "2 Corinthians": {
        5: {
            17: "Therefore if any man be in Christ, he is a new creature: old things are passed away; behold, all things are become new.",
            21: "For he hath made him to be sin for us, who knew no sin; that we might be made the righteousness of God in him.",
        },
        12: {
            9: "And he said unto me, My grace is sufficient for thee: for my strength is made perfect in weakness. Most gladly therefore will I rather glory in my infirmities, that the power of Christ may rest upon me.",
        },
    },
    "Galatians": {
        2: {
            20: "I am crucified with Christ: nevertheless I live; yet not I, but Christ liveth in me: and the life which I now live in the flesh I live by the faith of the Son of God, who loved me, and gave himself for me.",
        },
        5: {
            22: "But the fruit of the Spirit is love, joy, peace, longsuffering, gentleness, goodness, faith,",
            23: "Meekness, temperance: against such there is no law.",
        },
    },
    "Ephesians": {
        2: {
            8: "For by grace are ye saved through faith; and that not of yourselves: it is the gift of God:",
            9: "Not of works, lest any man should boast.",
            10: "For we are his workmanship, created in Christ Jesus unto good works, which God hath before ordained that we should walk in them.",
        },
        6: {
            10: "Finally, my brethren, be strong in the Lord, and in the power of his might.",
            11: "Put on the whole armour of God, that ye may be able to stand against the wiles of the devil.",
        },
    },
    "Philippians": {
        4: {
            4: "Rejoice in the Lord alway: and again I say, Rejoice.",
            6: "Be careful for nothing; but in every thing by prayer and supplication with thanksgiving let your requests be made known unto God.",
            7: "And the peace of God, which passeth all understanding, shall keep your hearts and minds through Christ Jesus.",
            8: "Finally, brethren, whatsoever things are true, whatsoever things are honest, whatsoever things are just, whatsoever things are pure, whatsoever things are lovely, whatsoever things are of good report; if there be any virtue, and if there be any praise, think on these things.",
            13: "I can do all things through Christ which strengtheneth me.",
            19: "But my God shall supply all your need according to his riches in glory by Christ Jesus.",
        },
    },
    "Hebrews": {
        4: {
            12: "For the word of God is quick, and powerful, and sharper than any twoedged sword, piercing even to the dividing asunder of soul and spirit, and of the joints and marrow, and is a discerner of the thoughts and intents of the heart.",
        },
        11: {
            1: "Now faith is the substance of things hoped for, the evidence of things not seen.",
            6: "But without faith it is impossible to please him: for he that cometh to God must believe that he is, and that he is a rewarder of them that diligently seek him.",
        },
        12: {
            1: "Wherefore seeing we also are compassed about with so great a cloud of witnesses, let us lay aside every weight, and the sin which doth so easily beset us, and let us run with patience the race that is set before us,",
            2: "Looking unto Jesus the author and finisher of our faith; who for the joy that was set before him endured the cross, despising the shame, and is set down at the right hand of the throne of God.",
        },
        13: {
            5: "Let your conversation be without covetousness; and be content with such things as ye have: for he hath said, I will never leave thee, nor forsake thee.",
            8: "Jesus Christ the same yesterday, and to day, and for ever.",
        },
    },
    "James": {
        1: {
            2: "My brethren, count it all joy when ye fall into divers temptations;",
            3: "Knowing this, that the trying of your faith worketh patience.",
            5: "If any of you lack wisdom, let him ask of God, that giveth to all men liberally, and upbraideth not; and it shall be given him.",
        },
    },
    "1 Peter": {
        5: {
            7: "Casting all your care upon him; for he careth for you.",
        },
    },
    "1 John": {
        1: {
            9: "If we confess our sins, he is faithful and just to forgive us our sins, and to cleanse us from all unrighteousness.",
        },
        4: {
            8: "He that loveth not knoweth not God; for God is love.",
            19: "We love him, because he first loved us.",
        },
    },
    "Revelation": {
        3: {
            20: "Behold, I stand at the door, and knock: if any man hear my voice, and open the door, I will come in to him, and will sup with him, and he with me.",
        },
        21: {
            4: "And God shall wipe away all tears from their eyes; and there shall be no more death, neither sorrow, nor crying, neither shall there be any more pain: for the former things are passed away.",
        },
    },
}


def create_database(db_path):
    """Create the Bible database with schema and sample data."""

    # Remove existing database if present
    if os.path.exists(db_path):
        os.remove(db_path)

    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    print(f"Creating database: {db_path}")

    # Create books table
    cursor.execute("""
        CREATE TABLE books (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL UNIQUE,
            abbreviation TEXT,
            testament TEXT,
            chapters INTEGER
        )
    """)

    # Create verses table
    cursor.execute("""
        CREATE TABLE verses (
            id INTEGER PRIMARY KEY,
            book_id INTEGER NOT NULL,
            chapter INTEGER NOT NULL,
            verse INTEGER NOT NULL,
            text TEXT NOT NULL,
            translation TEXT NOT NULL DEFAULT 'KJV',
            FOREIGN KEY (book_id) REFERENCES books(id)
        )
    """)

    # Create indexes for faster queries
    cursor.execute("CREATE INDEX idx_verses_reference ON verses(book_id, chapter, verse)")
    cursor.execute("CREATE INDEX idx_verses_translation ON verses(translation)")
    cursor.execute("CREATE INDEX idx_verses_book_chapter ON verses(book_id, chapter)")

    # Insert books
    print("Inserting book data...")
    for i, (name, abbrev, testament, chapters) in enumerate(BOOKS, 1):
        cursor.execute(
            "INSERT INTO books (id, name, abbreviation, testament, chapters) VALUES (?, ?, ?, ?, ?)",
            (i, name, abbrev, testament, chapters)
        )

    # Build book name to ID mapping
    cursor.execute("SELECT id, name FROM books")
    book_ids = {name: id for id, name in cursor.fetchall()}

    # Insert sample verses
    print("Inserting verse data...")
    verse_count = 0
    for book_name, chapters in SAMPLE_VERSES.items():
        book_id = book_ids.get(book_name)
        if not book_id:
            print(f"Warning: Unknown book '{book_name}'")
            continue

        for chapter, verses in chapters.items():
            for verse_num, text in verses.items():
                cursor.execute(
                    "INSERT INTO verses (book_id, chapter, verse, text, translation) VALUES (?, ?, ?, ?, ?)",
                    (book_id, chapter, verse_num, text, "KJV")
                )
                verse_count += 1

    print(f"Inserted {verse_count} verses")

    # Create full-text search virtual table
    print("Creating full-text search index...")
    cursor.execute("""
        CREATE VIRTUAL TABLE verses_fts USING fts5(
            text,
            content=verses,
            content_rowid=id
        )
    """)

    # Populate FTS table
    cursor.execute("INSERT INTO verses_fts(verses_fts) VALUES('rebuild')")

    conn.commit()
    conn.close()

    print(f"Database created successfully: {db_path}")
    print(f"  - {len(BOOKS)} books")
    print(f"  - {verse_count} verses")

    # Show file size
    size = os.path.getsize(db_path)
    if size < 1024:
        print(f"  - Size: {size} bytes")
    elif size < 1024 * 1024:
        print(f"  - Size: {size / 1024:.1f} KB")
    else:
        print(f"  - Size: {size / (1024 * 1024):.1f} MB")


def main():
    if len(sys.argv) > 1:
        output_path = sys.argv[1]
    else:
        output_path = "bible.db"

    create_database(output_path)

    # Verify database
    print("\nVerifying database...")
    conn = sqlite3.connect(output_path)
    cursor = conn.cursor()

    cursor.execute("SELECT COUNT(*) FROM books")
    book_count = cursor.fetchone()[0]

    cursor.execute("SELECT COUNT(*) FROM verses")
    verse_count = cursor.fetchone()[0]

    cursor.execute("SELECT COUNT(DISTINCT translation) FROM verses")
    translation_count = cursor.fetchone()[0]

    print(f"  Books: {book_count}")
    print(f"  Verses: {verse_count}")
    print(f"  Translations: {translation_count}")

    # Test a lookup
    print("\nTest lookup (John 3:16):")
    cursor.execute("""
        SELECT b.name, v.chapter, v.verse, v.text
        FROM verses v
        JOIN books b ON v.book_id = b.id
        WHERE b.name = 'John' AND v.chapter = 3 AND v.verse = 16
    """)
    result = cursor.fetchone()
    if result:
        print(f"  {result[0]} {result[1]}:{result[2]}")
        print(f"  {result[3]}")
    else:
        print("  Not found")

    # Test FTS search
    print("\nTest search ('love'):")
    cursor.execute("""
        SELECT b.name, v.chapter, v.verse, substr(v.text, 1, 50) || '...'
        FROM verses v
        JOIN books b ON v.book_id = b.id
        JOIN verses_fts fts ON v.id = fts.rowid
        WHERE verses_fts MATCH 'love'
        LIMIT 3
    """)
    for row in cursor.fetchall():
        print(f"  {row[0]} {row[1]}:{row[2]} - {row[3]}")

    conn.close()


if __name__ == "__main__":
    main()
