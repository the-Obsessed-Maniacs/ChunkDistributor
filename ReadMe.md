# Chunk-Distribution-Calculator
This algorithm is nothing special. A simple brute force method, trying all possible solutions.  In an ordered way, though.

## What is it about - what's the goal, here?
The starting situation is:
- We have some chunks of data, all of them less than a page (i.e. 256 bytes) in size. Their location in memory doesn't matter, as long as it is known.
- We also have some blocks of memory. Most likely code.  As we don't really know those code block sizes in advance when filling them with code (the assembler will be the first to really know these block sizes), it would be nice if the assembler could assist in filling up those code pages where possible.

The goal is to fill those pages of memory as tightly as possible with the available data chunks, which turned out to be a nice math problem.

That is about the UI You can see below:
- Data chunks simply have a size, so a comma-separated list is used to know our data chunks.
- Pages have a start address and may also have a ":size" appended, if not the full page is to be filled
  (the generator simply randomizes start addresses, that's sufficient for now.) 
## Motivation
There are situations, when a Democoder has to place specific pieces of code at specific memory locations. The size of those pieces may be calculated by hand to fill those pages with data or other code, but why?  That'd be a tedious job ...

On one hand, most code does need some data. Many pieces only need very little chunks, or "scratch spaces", but there's no place (or need) for it in the zeropage - why not use those little data chunks to fill up code-piece-clobbered pages?

On the other hand, other pieces of code need tables, some larger, some smaller \- also a good question, where to place those in memory \- maybe not really ... maybe it can simply be solved mathematically with all that processing power we didn't have back in 1982 ... :)

### Original motivation
Originally, I thought about data chunk distribution because I had problems creating a proper music packer for my "ATMDS v3.2" SID Music Editor.  I guess this is one reason, why nobody else ever used it.  The other is the editor itself, I also guess ;)

Anyhow, when repeatedly failing to create a packed music that sounds exactly like the version created inside the editor, I realized one very important fact:

`page boundary crossings`

- while accessing `misaligned data` - they add up quickly and ...
- `introduced` by relocations of the player to "uneven" addresses (i.e. not on a page boundary), including:
  - branches over page boundaries ...
  - the player variables, now positioned to span over a page boundary ...
- `removed` by relocations of the player to "uneven" addresses - i.e. branches, that normally cross page boundaries in the editor ...

... all of them `disturb the inner timing of the player`, which leads to the music not sounding the same anymore.

There's not much to think about, the solution's on the hand:
- Do not relocate "stupidly tight" - i.e. choose a page boundary.
- get rid of misaligned data.

## Basic Approach
We actually need two distinct classes of definitions:
- Data / Code blocks to distribute - let's call'em `"chunks"`
- Memory pages that are partially clobbered and have some space left to fill - i.e.`"pages"`

This is not really all information we need.  Maybe we have more chunks than memory?  Maybe the algorithm does not find a perfect solution in due time?

Well, automagically we'll just say: if there's more data than pages to unclobber, just continue writing chunks after the last page, now filling full pages.

It may be helpfull to allow penalties - like if it just doesn't work out to fill a page, just accept that a few bytes are wasted.  Maybe 4 is a good value?

With all the music data I tested I had to find out: *one byte sometimes makes a difference.*  **Even if it's a byte more ;)**  In the first case, the algorithm finished within the blink of an eye, if it has only one byte more on the first page, it can calculate into oblivion and not find a perfect solution.

### Algorithm
P.s. any code snippets following may look C(++)'ish, but are meant as theoretical code ideas.

#### Preparation
- calculate a list of IDs to the chunks, size-sorted, descending - `avail[]`
- calculate a list of pages, size-sorted ascending - `pages[]`
  - a `page` should contain:
     - its original `start_address`
     - its current `bytes_left`
  - useful:
     - include a solution map (or multimap) in a page
     - calculate the last page (address ascending), and append an element for it to `pages[]` (simplifies the code later on)
- set indices into those lists `a_id` + `p_id`, pointing to the start
- set up runner variable `bytes_left` (or use the `bytes_left` of a `page`)
- prepare an empty list of integer lists for the `result` (or include them in pages, as mentioned earlier)

`a_id` and `p_id`, as well as `avail[]` and `pages[]` should be kind of "global" - i.e. all parts in the algorithm see the same pieces of memory.

#### Helper functions
With just 3 helper functions, the final algorithm looks pretty much the same in many languages.

##### <u>sort\_into\_avail()</u>
Takes the index of a chunk and inserts it into `avail` at the correct position according to its size, increments `a_id` afterwards, so it already points to the next element to check.

	void sort_into_avail( int c_id )
	{
		a_id = 0
		while ( ( a_id < avail.size )
			&& ( chunks[ c_id ].size < chunks[ avail[ a_id ] ].size )	)	++a_id
		avail.insert( a_id++, c_id );
	}

##### <u>take\_available()</u>
Takes the current available chunk id from the available list and puts it into the current solution.

Updates `bytes_left` accordingly, `a_id` already points to the next smaller available chunk id to check due to the list removal.

If `bytes_left` ran down to zero, increment `p_id` and reset `a_id` to "0", reinitialize `bytes_left` from that next page.

	void take_available()
	{
		id = avail.take_at( a_id )
		pages[ p_id ].solution.append( id )
		if ( ( pages[ p_id ].bytes_left -= chunks[ id ].size ) == 0 )
			++p_id, a_id = 0
		bytes_left = pages[ p_id ].bytes_left
	}

##### <u>make\_available()</u>
This is the backtracking step.  It makes the latest chunk id of the current solution available again, by sorting it into the `avail[]` list.  `a_id` is set to the position following the insertion.

In case the current page's solution already is empty, the backtracking step goes deeper, and also decreases `p_id` if it is not zero, then taking the last solution element back to `avail`.

If it is zero, the case `no soultion` has happened and the algorithm shall end - returning "false".  In any other case, return true.

    bool make_available()
    {
		if ( pages[ p_id ].solution.isEmpty )
			if ( p_id == 0 )					return false
			else 								--p_id

		assert ( ! pages[ p_id ].solution.isEmpty ) // safeguard, should never happen!

		c_id = pages[ p_id ].solution.takeLast()
		bytes_left = ( pages[ p_id ].bytes_left += chunks[ c_id ].size )
		sort_into_avail( c_id )
		return true
	}


### Main loop

    do {
    	while ( a_id < avail.count() && bytes_left )
    		if ( bytes_left >= chunk[ avail[ a_id ] ].size )    take_available()
    		else                                                ++a_id
    } until ( avail.isEmpty() || !make_available() )

## Future?
I feel it would be a good idea to integrate this into [KickAssembler](https://www.theweb.dk/KickAssembler).  What exactly would be needed?

- directive `.datastore` ... declares the rest of the current page as a data store
- directive `.datachunk` ... defines a data chunk

The 1st directive `.datastore` should be pretty much self-explaining.  It doesn't take any more parameters than the implicit `*` to declare "the rest of a page to be fillable".  Anyhow, one could argue that also situations may arise, where "some bytes at the end of a page have to be just there" - like a GhostByte f.e.

For those cases, why not add a size override option? Like: `.datastore [bytes=(start_of_important_bytes - *)]`

The 2nd directive should offer a few more possibilities.  During my research and testing it became crystal clear to me, that `.datachunk`s could also contain code.  There's just no reason against it.  So maybe just going with KickAssembler's `.memblock` or extending it may also be an option.  For now, let's just not worry about a name, it's about a concept.

If a data chunk contains table data, it may be precalculated, imported, or initialized in some other way, or simply declared without initialization for realtime calculation.  The importance lies in labels.  The datachunk will most likely only be usable if its addresses are known thru its labels.

I suggest those variation/options to `.datachunk`:

	.datachunk [    data=List(Bytes),
	                binary_filename="fn", binary_offset=offs, binary_size=byte_count,
	                labels=Hashtable(String,int)					  ]
   - option: `data` using this option, the data is given as a KickAssembler List of bytes - this way the chunk's content and size may change until the final pass
   - option: `binary*` imports the data in binary from a file
      - `binary_filename` is neccessary
      - `binary_offset` and `binary_size` are optional, in case you use a prg file or just need a few bytes of the file
   - option: `labels` - this optional option is used to implicitly declare global labels at specific positions inside the data chunk.  The keys of this Hashtable are the label names (and should suffice the respective constraints), the values are offsets into this data chunk.  I.e. `labels = Hashtable().put( "start_of_chunk", 0 )` - hope that explains the idea.

...

   	.datachunk brot_machen:{
   				lda wurst,x
				sta brot
				rts
		@wurst:	.by wurst1, wurst2 	}

   	=== [same as]

   	.datachunk {
   		@brot_machen:
   				lda wurst,x
				sta brot
				rts
		@wurst:	.by wurst1, wurst2 	}

This is the second variation.  Simply declaring the following block as a `.datachunk` ... with one global label declaring its start in standard KickAssembler syntax.  And another global label being implicitly declared inside the block.  (In the 2nd version, both declared implicitly inside the block)

Well, I actually believe these curly braces of a chunk directive should not open a new scope.  But I guess this opinion is arguable.

Maybe the obvious choice is to make two different directives: `.datachunk` and `.codechunk` ;)