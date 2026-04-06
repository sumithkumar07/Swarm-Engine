"""
swarm V10 Training Data Preparation
========================================
Downloads public domain English text, cleans it, builds vocabulary,
and prepares training data for the Master Brain.

Usage: python prepare_training_data.py
Output: training_data.txt (~10-50MB), vocab.txt (5000 words)
"""

import os
import re
import urllib.request
from collections import Counter

# --- Public domain text sources (Project Gutenberg) ---
SOURCES = [
    # Short stories & essays (good for conversational patterns)
    ("https://www.gutenberg.org/cache/epub/1342/pg1342.txt", "Pride and Prejudice"),
    ("https://www.gutenberg.org/cache/epub/11/pg11.txt", "Alice in Wonderland"),
    ("https://www.gutenberg.org/cache/epub/1661/pg1661.txt", "Sherlock Holmes"),
    ("https://www.gutenberg.org/cache/epub/84/pg84.txt", "Frankenstein"),
    ("https://www.gutenberg.org/cache/epub/98/pg98.txt", "A Tale of Two Cities"),
    ("https://www.gutenberg.org/cache/epub/1232/pg1232.txt", "The Prince (Machiavelli)"),
    ("https://www.gutenberg.org/cache/epub/2701/pg2701.txt", "Moby Dick"),
    ("https://www.gutenberg.org/cache/epub/74/pg74.txt", "Tom Sawyer"),
    ("https://www.gutenberg.org/cache/epub/1400/pg1400.txt", "Great Expectations"),
    ("https://www.gutenberg.org/cache/epub/46/pg46.txt", "A Christmas Carol"),
]

def download_text(url, name):
    """Download a text file from URL."""
    print(f"  Downloading: {name}...", end=" ")
    try:
        req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
        with urllib.request.urlopen(req, timeout=30) as resp:
            text = resp.read().decode("utf-8", errors="ignore")
        print(f"OK ({len(text):,} chars)")
        return text
    except Exception as e:
        print(f"FAILED ({e})")
        return ""

def clean_gutenberg(text):
    """Remove Project Gutenberg headers/footers and clean text."""
    # Remove header (everything before "*** START")
    start = text.find("*** START")
    if start != -1:
        newline = text.find("\n", start)
        text = text[newline+1:]

    # Remove footer (everything after "*** END")
    end = text.find("*** END")
    if end != -1:
        text = text[:end]

    # Basic cleaning
    text = re.sub(r'\r\n', '\n', text)           # Normalize newlines
    text = re.sub(r'\n{3,}', '\n\n', text)       # Collapse excess newlines
    text = re.sub(r'[^\x20-\x7E\n]', '', text)   # ASCII only
    return text.strip()

def build_vocab(text, max_vocab=5000):
    """Build a word vocabulary from text."""
    # Tokenize: split on whitespace, lowercase, strip punctuation
    words = re.findall(r"[a-z]+(?:'[a-z]+)?", text.lower())
    freq = Counter(words)

    # Keep the most common words
    # Reserve 4 slots for special tokens
    common = freq.most_common(max_vocab - 4)

    vocab = ["[PAD]", "[UNK]", "[START]", "[END]"]
    vocab += [word for word, count in common]

    return vocab

def main():
    print("=" * 60)
    print("swarm V10 — TRAINING DATA PREPARATION")
    print("=" * 60)

    # Step 1: Download texts
    print("\n[STEP 1] Downloading training corpus...")
    all_text = ""
    for url, name in SOURCES:
        raw = download_text(url, name)
        if raw:
            cleaned = clean_gutenberg(raw)
            all_text += cleaned + "\n\n"

    if len(all_text) < 10000:
        print("\n[ERROR] Not enough text downloaded. Check your internet connection.")
        return

    # Step 2: Save training data
    print(f"\n[STEP 2] Saving training data... ({len(all_text):,} characters)")
    with open("training_data.txt", "w", encoding="utf-8") as f:
        f.write(all_text)
    size_mb = os.path.getsize("training_data.txt") / (1024 * 1024)
    print(f"  Saved: training_data.txt ({size_mb:.1f} MB)")

    # Step 3: Build vocabulary
    print(f"\n[STEP 3] Building vocabulary (top 5000 words)...")
    vocab = build_vocab(all_text, max_vocab=5000)
    with open("vocab.txt", "w", encoding="utf-8") as f:
        for word in vocab:
            f.write(word + "\n")
    print(f"  Saved: vocab.txt ({len(vocab)} words)")

    # Step 4: Stats
    words = re.findall(r"[a-z]+(?:'[a-z]+)?", all_text.lower())
    known = sum(1 for w in words if w in set(vocab))
    coverage = known / len(words) * 100
    print(f"\n[STATS]")
    print(f"  Total words in corpus: {len(words):,}")
    print(f"  Vocabulary size: {len(vocab)}")
    print(f"  Coverage: {coverage:.1f}% of all words are in vocabulary")
    print(f"  Training data size: {size_mb:.1f} MB")
    print(f"\n{'=' * 60}")
    print("READY. Next step: compile V10 engine and train.")
    print("=" * 60)

if __name__ == "__main__":
    main()
