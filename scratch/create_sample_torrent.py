import hashlib

def bencode(val):
    if isinstance(val, int):
        return f"i{val}e".encode('utf-8')
    elif isinstance(val, str):
        b = val.encode('utf-8')
        return f"{len(b)}:".encode('utf-8') + b
    elif isinstance(val, bytes):
        return f"{len(val)}:".encode('utf-8') + val
    elif isinstance(val, list):
        res = b"l"
        for item in val:
            res += bencode(item)
        res += b"e"
        return res
    elif isinstance(val, dict):
        res = b"d"
        for k in sorted(val.keys()):
            res += bencode(k) + bencode(val[k])
        res += b"e"
        return res
    raise TypeError(f"Cannot bencode {type(val)}")

# Sample data content
content = b"BitTorrent educational client test content.\nThis file is used to verify torrent parsing, piece splitting, and SHA-1 piece validation.\n"
piece_len = 32
pieces = b""
for i in range(0, len(content), piece_len):
    chunk = content[i:i+piece_len]
    pieces += hashlib.sha1(chunk).digest()

torrent_dict = {
    "announce": "http://127.0.0.1:6969/announce",
    "comment": "Educational Sample Torrent",
    "created by": "BitTorrent C++20 Test Suite",
    "info": {
        "length": len(content),
        "name": "sample_data.txt",
        "piece length": piece_len,
        "pieces": pieces
    }
}

bencoded = bencode(torrent_dict)

with open("sample.torrent", "wb") as f:
    f.write(bencoded)

with open("sample_data.txt", "wb") as f:
    f.write(content)

print("Generated sample.torrent and sample_data.txt successfully!")
