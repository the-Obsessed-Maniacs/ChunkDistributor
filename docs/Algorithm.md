Chunk Distribution Visualizer -
description of algorithms and algorithm development
===================================================
---
## 1. Disambiguation

| word | description |
| :---: | :--- |
| chunk | A Block of data or code bytes.  For the part of finding the right place in memory, it's contents does not matter at all.  Our main interest is: its size in bytes.|
| page | A memory range to fill with chunks.  The name comes from the way 8bit processors looked at their memory, sliced into pages of 256 bytes.<br>A page has two important parameters:<br><li/> where does the free space start,<br><li/> and how many bytes are free?

---
## 2. very first approach: the basic algorithm
With its origin in music packing, the very first idea was a simple backtracker with a big simplification:
- only the first page has less than 256 bytes free, depending on the "end of player code"
  - depending on player version, first pages in the range of \$1864-\$18af were used

The bounds were also very simple:
- go page by page, fill them till no bytes are left.

### Variables:
- `available`: a list of chunk IDs, sorted by chunk size in descending order.
- `pages`: a list of the created pages, ascending order
  - contains `start address`, `bytes_left` and a list of the currently contained `chunk_ids`
  - initially, only the starting page is added
- `a_id`: current index into the `available` list - i.e. the next chunk to be tested
- `p_id`: index of the current page being processed (for page backtracking purposes)

### Initial state:

The list `available` contains all chunk IDs and gets sorted as described by chunk size in
descending order.  Only the starting page is added to `pages`, containing an empty solution.
Both IDs are zero-initialized.

### Flow Diagrams:
#### main iteration loop:
```mermaid
flowchart
	ST@{ shape: circ, label: "START" }-->
	|_a_id_ and _p_id_ are current|a0["increment iteration_count"]-->
    c0{"available not empty<br/>AND<br/>_bytes_left_ > 0"};
	c1{"does current<br/>chunk fit into<br/>current page?"};
	n1["increment _a_id_<br/>(i.e. point to next maybe<br/>smaller available chunk)"]-->j1((" "));
    n0{"available empty?"};
	n2{"_bytes_left_ == 0"};
    j2((" "));
	z((("SOLVED!")));
	c0-->|no|n0;
    c0-->|yes|c1;
	c1-->|no|n1;
    c1-->|yes|tac[["take available chunk"]]-->j1----->c0;

	n0------>|yes|z;
	n0-->|no|n2;
	n2-->|yes|sn[["switch to next page"]]-->j2;
	n2-->|no|bt[["do backtrack"]]-->j2;
    j2--->ST;
```
#### sub functions:
```mermaid
flowchart
	subgraph "take available chunk"
		y1a["remove chunk @position _a_id_ from _available_ list"]-->
		y1b["append chunk to current _page_"]-->
		y1c["update _bytes_left_ of current page."]-->
        y1end(["done"])
	end
```
```mermaid
flowchart
	subgraph "switch to next page"
        sn1["increment _p_id_"]-->
        sn2["reset _a_id_ (=0)"]-->
        snc{"_p_id_<br/> < <br/>pages.count"};
        snc-->|no|sn3["append empty page<br/>(->new current page)"]-->
        sn4["init _bytes_left_ from page"]-->
        snend(["done"])
        snc-->|yes|sn4;
	end
```
```mermaid
flowchart
	subgraph "do backtrack"
        btc0{"_a_id_ >= _available.count_<br/>AND<br/>current page has chunks"\}-->
        |yes|bt1["take last Chunk<br/>from current page"]-->
        bt2[["sort chunk into _available_"]]-->btc0;
        btc0-->|no|btc1{"_a_id_ < _available.count_"}-->
        |no|btc2{"_p_id_ > 0"}-->
        |yes|btpp1["decrement _p_id_<br/>(switch to previous page)"]-->
        btpp2["update _bytes_left_ from new current page"]-->btc0;
        btc2---->|no|algodone(["NO PERFECT SOLUTION POSSIBLE"])
        btc1----->|yes|btdone(["backtracking<br/>ok"])
	end
```
```mermaid
flowchart
	subgraph "sort chunk into _available_"
        scstart((("given: chunk<br/>/ chunk ID")))-->
		sc0["_a_id_ = 0"]-->
		sc1{"_a_id_ < _available.count_"}-->
        |yes|sc2{"chunk.bytes < _available[a_id].bytes_"}-->
        |yes|scl["increment _a_id_"]-->sc1
        sc2-->|no|sc3{"chunk.bytes > _available[a_id].bytes_"}-->
        |no|sc4{"chunk.prio < available[a_id].prio<br/>OR<br/>chunk.number > available[a_id].number"}-->|yes|scl
        sc3-->|yes|sci0["insert chunk_id into _available_ @ _a_id_"]-->
        sci1["increment _a_id_"]-->
        scend(["done"])
	end
```