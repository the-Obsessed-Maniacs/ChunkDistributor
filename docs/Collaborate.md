Chunk Distribution Visualizer - how to collaborate ?
====================================================
As I really found no good idea on which occasion the algorithm should change or f.e. allow some
threshold or even page boundary crossings, I decided to make it easy for anybody, to implement
their own ideas without much effort.
<!--TOC-->
  - [A short dive into the program](#a-short-dive-into-the-program)
    - [AlgoData.h](#algodata.h)
    - [AlgoGfx.h/cpp](#algogfx.hcpp)
    - [Algo.h/cpp](#algo.hcpp)
      - [AlgoRunner](#algorunner)
        - [protected Variables](#protected-variables)
        - [`current_state`](#current_state)
        - [`cur_btsleft_thresh`](#cur_btsleft_thresh)
        - [`AlgoRunner` function descriptions](#algorunner-function-descriptions)
          - [`init()`](#init)
          - [`sort_into_avail( int chunk_id )`](#sort_into_avail-int-chunk_id-)
          - [`take_available()`](#take_available)
          - [`make_available()`](#make_available)
          - [`QPair< quint16, quint16 > read_page_info( const QString tx ) const`](#qpair-quint16-quint16-read_page_info-const-qstring-tx-const)
          - [`quint16 read_maybehex( const QString tx ) const`](#quint16-read_maybehex-const-qstring-tx-const)
          - [`int bytes( int chunk_id ) const`](#int-bytes-int-chunk_id-const)
          - [`int prio( int chunk_id ) const`](#int-prio-int-chunk_id-const)
  - [`AlgoRunner`'s event loop](#algorunners-event-loop)
  - [deriving your `AlgoRunner`](#deriving-your-algorunner)
<!--/TOC-->
## A short dive into the program
- main.cpp is a simple Qt app starter
- PackerTester.h/cpp contains the whole UI, as well as the ATMDS3-Implementation (that's why it
runs much slower even in `silent` mode - first try without multithreading.)
- Stage.h/cpp contains some QGraphicsWidgets that are used for visualization
- AnimatePosition.h/cpp contains a CRTP-mix-in for animated QGraphicsObject movement.

All those code files rather belong to the first implementation (at least that is what makes them
a little crowded), and to application / UI control.

- AlgoData.h
- Algo.h/cpp
- AlgoGfx.h/cpp

Were kind of refactored out them.  I wanted to show off without any need, so I even created a Qt
Designer Plugin for the Algo Widget.  This may be usefull for some preconfiguration if somebody
derives an own algorithm.

| ℹ️ | I was planning a full refactoring of the application, but I am not quite convinced, yet, if that'd be a waste of time. This super-option of simply using the Qt Designer, adding a `Algo::Algo`-Widget and run preview seems really cool and a time-saver.|
|--|:--|

### AlgoData.h
... describes data structures:
- `AlgoPage`, which is the base to record the description about a page and at the same time keep
one record to work on and one to present a solution at the same time.
- `ResultCache` - declared as using a `QMap< quint16, AlgoPage>` (where the key is the start_address)
- `AlgoCom` - a communication structure for multithreaded working, contains:
  - a ResultCache
  - a mutable ReadWriteLock
  - and a boolean to signalize fresh data

It should be noted that access patterns to this communication structure ***have to be strictly obeyed!***
In clean words this means the ReadWriteLock shall be used:
- to protect access to the `.solution`-part of each result cache entry
- if it is important to "atomically" check for new data, when reading `freshData`
- when writing `freshData`

That way the communication is made thread-safe.

The ui thread will receive status signals upon which it may decide to display any current solution.
So normally, it doesn't need to check `freshData` at all, it gets a different trigger.  The ui may
only access the readWriteLock to acquire access to the data structure.  If successfull, it may read
all page's solution data, reset `freshData` to false and then release the readWriteLock.

The calculation thread on the other hand accesses the result cache all the time.  ___BUT___, well,
only the selection part and the current "bytes_left" in writing.  As soon as it has a solution
ready, it shall as well acquire the lock, but this time for writing. Then the `selection` will be
safely copied to the solution.  `freshData` is set to true and the lock released.

### AlgoGfx.h/cpp
Implements the QGraphicsObjects/Widgets needed to display solutions visually.  It also contains
the declaration of the CRTP-mix-in I wrote for animating the movement of QGraphicsObjects.

You may want to change stuff there, make it look nicer, whatever - I'd be glad in appreciation.  But for
mathematical purposes it doesn't matter, it's the boilerplate I made so we can *look* at how
different implementations *act*.

### Algo.h/cpp
Here's where the hot stuff happens ;)

First, the Algo-Widget is declared, providing the description, the group
boxes for simulation and generation, the animation group box and the QGraphicsView for visualization.
All Ui is internally interconnected to corresponding slots, so the widget already does its visual
housekeeping by itself.

For collaborating you do not have to care much about UI.  It's about a new `worker` and aforementioned
communication structure `AlgoCom`.  The Algo UI Widget owns such a structure to provide the scratch-
and reporting space.

#### AlgoRunner
Algo.h also defines `AlgoRunner`.  This class is pure virtual, as the `iterate()` method has no
base implementation.

This is the class you'd want to derive your algorithm implementation from.  And it really only needs
that single function and a CTor - of course you are free to implement what you think is neccessary.

##### protected Variables
All variables of `AlgoRunner` are declared protected, thus your derivate can use them just as if it
were your own variables.

| variable(s) | description |
| :--- | :--- |
| `com` | a reference to the communication structure<br/><ul><li/>any worker needs to be constructed with a `&AlgoCom`, so when accessing the memory, it writes to the data structure owned by `Algo`</ul> |
| `a_id` and `p_id` | idices of current available chunk and currently processed page |
| `chunks` | a simple list of integers, representing the chunk sizes given packed together with their priority (Bits 0..15: size. Bits 16..31 prio) |
| `avail` | sorted list of chunk_ids (i.e. positions inside `chunks`), sorted by chunk size in descending order |
| `pageOrder` | a sorted list of page indices, sorted by free space ascending |
| `badPages` | initially empty, used to rule out pages that simply cannot be filled<ul><li/>in that case, the page_id is removed from `pageOrder` and appended to `badPages`<li/>`p_id` does not change thru this operation, it simply points to the next page (or end) afterwards.|
| `current_state` | I'll go into detail below |
| `cnt_sel`, `cnt_unsel`<td rowspan="2">**some 64bit counters for operations**<ul><li/>count selection/deselection operations (make_available/take_available)<li/>`iteration` ... should be incremented upon entering `iterate()`<li/>`lastIt` ... remembers the last time a progress report had been emitted<ul><li/>Used to constrain progress reports / status changes to ~every 2<sup>14</sup> iterations.<li/>if not done, the Signals & Slots system slows down operations considerably!</ul></ul></td>
| `iteration`, `lastIt` | 
| `cur_btsleft_thresh` | how many bytes may be left before a page is considered as "filled"?<ul><li/>initially set to zero<li/>I added this lately, because I think this is the direction to go - more below.</ul>
  
##### `current_state`
I've been thinking about a good way to report current progress and state at once to not overload
Qt's Signal and Slot mechanism.  Well, as you can see from `lastIt`, it didn't even help to restrict
the data flow.

Anyhow, I wanted to be able to report some state as well as some counters at once.

So, `Algo` declares `Algo::State` as a 64bit enum containing the "real" state bits:
- `deeper_calc`
- `running_bit`
- `pause_bit`
- `init_bit`

Where `pause_bit` is the only one with kind of a double representation.  If the `running_bit` is
set, a set `pause_bit` signalizes that pausing the calculation was requested.

As soon as the worker wants to respond, it simply resets the `running_bit` and returns to its event
loop.  Then only the `pause_bit` stays set, which actually does mean "the algorithm is paused."

This pausing mechanism is built into `AlgoRunner` if your derrivate uses it as supposed (see below).

Then, `Algo::State` declares bit masks and a shift count:
- `page_mask`
- `step_mask`
- `bits_mask`
- `page_bits`

And finally there are some const expressions for separating `currentPage` and `currentStep`,
as well creating a state from bits and the current values.

So all in all the `AlgoRunner` derrivate reports state as (bits, currentPage, currentStep).

##### `cur_btsleft_thresh`
If not all pages can be filled to the brim, most likely wasting a few bytes hurts less than having
data cross page boundaries.  Thus at some point this threshold may be increased to have better
chances to fill pages with the available chunks.

***BTW:*** this is what the abort condition I am seeking should do... increase this threshold until
a solution can be found, or it is triggered again to increase the threshold.

---
##### `AlgoRunner` function descriptions
###### `init()`
`AlgoRunner` not only declares the data, it also prepares it thru the init function.  There are 2
virtual variations, because I already added a priority system to the chunks, which isn't used, yet,
except in coloring the blocks visually and it is respected by `sort_into_avail( chunk_id )` (i.e.
lowest prio-value comes first if two chunk sizes are equal).  For understanding: the highest prio
would be top of the list, thus its value (or, for that matter, prio-index) is 0.

The priorities are given as a String List.  Each string itself is interpreted as a comma-separated
list of chunk_ids.  The priority-value of a chunk equals the list-index of the line, where its id
is found.  If priorities are given but none for a specific chunk, that chunk receives the last
priority given plus one.  I.e. if you input { "0,1,2,3" }, those 4 chunks will have a prio-value of
"0" and all further chunks get "1".

If you would like to do different house-keeping, just implement your own `init()`.

###### `sort_into_avail( int chunk_id )`
Sorts a given chunk_id into the `avail` list, respecting priority and ID next to the primary sort
criterium "size".

###### `take_available()`
Takes the current available chunk, as pointed to by `a_id`, out of `avail` and appends it to the
current selection pointed to by `p_id` via `com.result[ pageOrder[ p_id ] ].selection`.  This is
the "selection operation".

`take_available()` returns true upon switching to the next page (i.e."Result ready!").

###### `make_available()`
This is the deselection operation - or a single backtracking step, if you want to put it that way.
The last current selection item is removed an reinserted into the `avail`-list.

`make_available()` also returns true upon a page change.  In that case it'd be a page backtracking
process, which will be handled as described in [Algorithm](/docs/Algorithm.md)

###### `QPair< quint16, quint16 > read_page_info( const QString tx ) const`
Short helper function for reading the page description strings (remember: "$addr+size" is allowed,
as well as "0xaddr" or even "0xaddr+$hexbytes").

###### `quint16 read_maybehex( const QString tx ) const`
Same kind of small helper - this is the one that tries to interpret a string as a number, taking
the different Hex notations into account.

###### `int bytes( int chunk_id ) const`
As I packed size and priority together, this is the split for size by id.

###### `int prio( int chunk_id ) const`
As I packed size and priority together, this is the split for priority by id.

## `AlgoRunner`'s event loop
This may be the most important explanation about how the AlgoRunner works with its thread's event
loop, although it should be a fast-running data processing linear code.

I could have left out the Signals and Slots and only use sync primitives (the existing lock would
actually suffice, already, but maybe another atomic int would be helpful), but I did not think
about that when implementing.  I just wrote a second thread with com object, just like I am used to
in Qt.

Anyhow.  ***The magic lies in the protected Signal `redo()`.*** It is connected internally upon
creation and provides a way to "take an event loop-round".  Every time an iteration process is
considered to be finished (and it is known, that another one will follow), `iterate()` should
gracefully end in emitting `redo()`.

This fires a queued signal and upon exiting `iterate()`, the thread event loop regains control.
It will now process all pending events.  Most likely the last event will be the `redo()` signal
just fired, which runs `pre_re_iterate()`.  This is the function doing all the checks and reporting
the current state if it makes sense.  After these obligations, it calls `iterate()` again.

So you see, by emitting `redo()` we make sure to process any events pending - which may be pause
requests, or something under Qt's hood.

## deriving your `AlgoRunner`
The first implementation `AlgoEngine` may serve as an example.  This is the current implementation
used and it shows, how you really only need to implement `iterate()`:

```cpp
	class AlgoEngine : public AlgoRunner
	{
		Q_OBJECT

	  public:
		explicit AlgoEngine( AlgoCom &com )
			: AlgoRunner( com )
		{}
		virtual ~AlgoEngine() = default;

	  public slots:
		void iterate() override;

	  protected:
		void iterate_complete();
	};
```
The protected `iterate_complete()` was added to keep the iterate function simple.  I kind of sourced
out a codepath that is taken only once (as the name suggests, it's a finishing func ;)).

It tries to compute a "completed percentage" from chunks distributed vs. pages filled and emits the
final report with these numbers.

So, most likely you can simply copy & paste from the code block above and replace `AlgoEngine` with
your classname - header file finished ;)

And ... actually, there you go: that's how `AlgoEngine::iterate()` currently looks like:
```cpp
	void AlgoEngine::iterate()
	{
		++iteration;
		do {						// outer loop: backtracking, inner loop: chunk selection
			while ( a_id < avail.count()
					&& com.result[ pageOrder[ p_id ] ].bytes_left > cur_btsleft_thresh )
				if ( com.result[ pageOrder[ p_id ] ].bytes_left >= chunks[ avail[ a_id ] ] )
				{
					if ( take_available() ) 
						if ( p_id < pageOrder.count() )
							return emit redo(); // next page -> create a new iteration!
						else return iterate_complete();
				} else ++a_id;
		} while ( !avail.isEmpty() && !make_available() );

		if ( avail.isEmpty() )
		{
			emit page_finished();
			emit final_solution( iteration, cnt_sel, cnt_unsel, 1.0 );
		} else {	// We didn't finish, so backtracking must have jumped a page back
			if ( pageOrder.isEmpty() || p_id >= pageOrder.count() )
				emit final_solution( iteration, cnt_sel, cnt_unsel, avail.isEmpty() );
			else emit redo();
		}
	}
```
---
So all in all you can use these two code blocks to start your own derrivate of `AlgoRunner`.  But
how to get it into the application?

Now `Algo` simply checks if it knows how to create an AlgoRunner as soon as a simulation shall be
started.  There is `Algo::createRunner`, which initiates to nullptr and can be changed using
`setCreateRunner()`. It needs a function pointer to a creator function.  As `AlgoRunner` needs a
`AlgoCom &` for construction, this must be the parameter a creator function accepts:
```cpp
	using RunnerCreatorFunc = std::function< AlgoRunner *( AlgoCom & ) >;
```
Please also have a look at "PackerTester.cpp:94":
```cpp
	t_algo->setRunnerCreator( []( Algo::AlgoCom &com_obj )
							  { return new Algo::AlgoEngine( com_obj ); } );
```
This is the way, how currently a creator function is set up.  As you can see, the given lambda
expression simply allocates a new instance of `AlgoEngine` on the heap, constructs it with the
given parameter and returns a pointer to it.

Which makes the current UI static ...

... At this point I must admit I have not yet done all that's possible to integrate another algorithm
seamlessly.  Steps that will follow (if I can make it work with Qt, that is):
- add a "Algorithm combo-Box" to Algo::Ui
- integrate a self-registering factory into Algo

That way in the future it will be sufficient, to simply derive off AlgoRunner with a specific
method and after compiling, it simply is there, listed inside that combo box.  That way after
compiling and creating a Designer DLL (CMake target "DesignerPlugin"), the whole would be even
runnable from within Qt Designer using the "preview" functionality, so not even an application
around would be neccessary.  Future music, gotto go implement it :smile: