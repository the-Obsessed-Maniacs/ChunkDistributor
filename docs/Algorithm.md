Chunk Distribution Visualizer -
description of algorithms and algorithm development
===================================================
<!--TOC-->
  - [1. Disambiguation](#1.-disambiguation)
  - [2. very first approach: the basic algorithm](#2.-very-first-approach-the-basic-algorithm)
    - [Variables:](#variables)
    - [Initial state:](#initial-state)
    - [Flow Diagrams:](#flow-diagrams)
      - [main iteration loop:](#main-iteration-loop)
      - [sub functions:](#sub-functions)
  - [3. First abstraction - free page sizes](#3.-first-abstraction-free-page-sizes)
<!--/TOC-->---
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
        scstart((("given: chunk<br/>/ chunk ID<br/>_a_id_ = 0")))--->
		sc1{"_a_id_ < _available.count_"}--->
        |yes|sc2{"chunk.bytes < _available[a_id].bytes_"}--->
        |yes|sc3["increment _a_id_"]--->sc1

        sc2-->|no|sc4{"chunk.bytes > _available[a_id].bytes_"}--->
        |no|sc5{"chunk.prio < _available[a_id].prio_<br/>OR<br/>chunk.number > _available[a_id].number_"}--->
        |yes|sc3

        sc4--->|yes|sci0["insert chunk_id into _available_ @ _a_id_"]--->
        sci1["increment _a_id_"]--->scend(["done"])

        sc1--->|yes|sci0

        sci0 <---|no|sc5
	end
```
---
## 3. First abstraction - free page sizes
The first iteration is about removing the binding to music packing.  I.e. the fixed bound that only
the first pages has less that 256 bytes free needs to be removed.

Also this introduces the possibility that the smallest page(s) leave(s) too little space to be
filled with chunks.  This was nearly impossible with music data, as the instrument tables had a max
length of 32 by design, speed-tables and init-data were even shorter, and I created no player that
left less than $50 bytes in its last page.

Thus the initial situation changes:
- there need to be more than one list of pages
  - `pages`: a list or map of all pages configured
    - `page_id`: index into this list
  - `available_pages`: a list of page_ids, sorted by free space ascending
  - `impossible_pages`: initially empty, will contain page_ids of pages that cannot be filled with
the available chunks

And we also get a new final state: `ALL_PAGES_FILLED`.  This is the "finish"-case, where there is
more data (i.e. chunks) available, than can be placed successfully inside the available pages.

Question arises, what to do in that case:
- As chunks must be left over, it could continue behind
the highest pages to distribute chunks as if those pages were marked "256 bytes free", just like
the music packer did.
- On the other hand the work has succeeded, the gaps are as filled as possible.

### 3.1. changed flow charts
To accommodate for the second "success"-option, in which all fillable given pages are filled, 
`switch to next page` needs an update so it can return the two states:
```mermaid
flowchart
	subgraph "switch to next page"
        sn1(((Start)))-->
        sn2["increment _p_id_<br/>reset _a_id_ (=0)"]-->
        snc{"_p_id_<br/> < <br/>_available_pages_.count"};
        snc-->|yes|sn4["init _bytes_left_ from page"]-->
        snend(["page switch<br/>complete"])
        snc-->|no|sn3(["ALL_PAGES_FILLED"])
	end
```
---
Backtracking needs to take care of sorting out pages we simply cannot fill. And obviously, the
function now returns an indicator if a page change occurred during backtracking:
```mermaid
flowchart
	subgraph "do backtrack"
        bt0([start backtracking])-->
        btc3{"current page is empty<br/>i.e. no chunks selected"}-->
        |yes|btc2{"_p_id_ > 0"}-->
        |yes|bt3["decrement _p_id_<br/>(switch to previous available page)"]-->
        btpp2["update _bytes_left_ from new current page<br/>(_available_pages_[ _p_id_ ])"]-->
        btdc(["backtracking ok<br/>page changed"])

        btc2-->
        |no|btrp["take page_id from _available_pages_[_p_id_]<br/><br/>append page_id to _impossible_pages_"]-->
        btpp2

        btc3-->
        bt1["take last Chunk from current page"]-->bt11["update _bytes_left_ of current page"]-->
        bt2[["sort taken chunk into _available_"]]-->
        btdone(["backtracking ok<br/>no page change"])
	end
```
The main iteration changes as follows:
```mermaid
flowchart
    start(((Start)))--->
	|_a_id_ and _p_id_ are current|a0["increment _iteration_count_"]--->
    c0{"_a_id_ < _available.count_<br/>AND<br/>_bytes_left_ > 0"}
    c0--->|yes|c1{"does current<br/>chunk fit into<br/>current page?"}
    c1--->|yes|tac[["take available chunk"]]
    c1--->|no|n1["increment _a_id_<br/>(i.e. point to next maybe<br/>smaller available chunk)"]
    c2--->|yes|c3{"_p_id_ < _available_pages.count_ ?"}
	z((("SOLVED!")))

    tac--->c2{"did page change?"}--->|no|j1(( ))--->c0
    n1--->j1
    c3--->|yes|j2(( ))--->start
    c3--->|no|z

    c0--->|no|c4{"is _available_ empty?"}--->|yes|z
    c4--->|no|bt[["do_backtrack"]]--->btc{"did current page change?"}
    btc--->|no|j1
    btc--->|yes|c5{"is _available_pages_ empty?"}--->|yes|z
    c5--->|no|j2
```
