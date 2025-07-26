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
  - [WorkerBase](#workerbase)
    - [protected Variables](#protected-variables)
      - [`current_state`](#current_state)
      - [`cur_btsleft_thresh`](#cur_btsleft_thresh)
    - [`WorkerBase` function descriptions](#workerbase-function-descriptions)
      - [`init()`](#init)
      - [`sort_into_avail( int chunk_id )`](#sort_into_avail-int-chunk_id-)
      - [`take_available()`](#take_available)
      - [`make_available()`](#make_available)
    - [`WorkerBase`'s event loop](#workerbases-event-loop)
  - [deriving your `WorkerBase`](#deriving-your-workerbase)
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

---
## WorkerBase
Algo.h also defines a factory and `WorkerBase`.  This class is pure virtual, as the `iterate()`
method has no base implementation.

To derive another `WorkerBase` you simply do as `AlgoRunner0.h/cpp` shows.  This is important,
because I used a self-registering factory pattern
([see Nir Friedman's Blog](https://www.nirfriedman.com/2018/04/29/unforgettable-factory/)).

But first, more info on the class and its capabilities (or jump directly to: [deriving your `Worker`](#deriving-your-`Worker`)):

### protected Variables
All variables of `WorkerBase` are declared protected, thus your derivate can use them just as if it
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
  
#### `current_state`
I've been thinking about a good way to report current progress and state at once to not overload
Qt's Signal and Slot mechanism.  Well, as you can see from `lastIt`, it didn't even help to
restrict the data flow by sending only 64bit every time state is reported.

So, the namespace declares `Algo::State` as a 64bit enum containing the "real" state bits:
- `deeper_calc`
- `running_bit`
- `pause_bit`
- `init_bit`

Where `pause_bit` is the only one with kind of a double representation.  If the `running_bit` is
set, a set `pause_bit` signalizes that pausing the calculation was requested.

As soon as the worker wants to respond, it simply resets the `running_bit` and returns to its event
loop.  Then only the `pause_bit` stays set, which actually does mean "the algorithm is paused."

This pausing mechanism is built into `WorkerBase` if your derrivate uses it as supposed (see below).

Then, `Algo::State` declares bit masks and a shift count:
- `page_mask`
- `step_mask`
- `bits_mask`
- `page_bits`

And finally there are some const expressions for separating `currentPage` and `currentStep`,
as well creating a state from bits and the current values.

So all in all the `WorkerBase` derrivate reports state as (bits, currentPage, currentStep).

#### `cur_btsleft_thresh`
If not all pages can be filled to the brim, most likely wasting a few bytes hurts less than having
data cross page boundaries.  Thus at some point this threshold may be increased to have better
chances to fill pages with the available chunks.

***BTW*** this is what the abort condition I am seeking should do: increase this threshold until
a solution can be found, or it is triggered again to increase the threshold another time.

---
### `WorkerBase` function descriptions
#### `init()`
`WorkerBase` not only declares the data, it also prepares it thru the init function.  There are 2
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

#### `sort_into_avail( int chunk_id )`
Sorts a given chunk_id into the `avail` list, respecting priority and ID next to the primary sort
criterium "size".

#### `take_available()`
Takes the current available chunk, as pointed to by `a_id`, out of `avail` and appends it to the
current selection pointed to by `p_id` via `com.result[ pageOrder[ p_id ] ].selection`.  This is
the "selection operation".

`take_available()` returns true upon switching to the next page (i.e."Result ready!").

#### `make_available()`
This is the deselection operation - or a single backtracking step, if you want to put it that way.
The last current selection item is removed an reinserted into the `avail`-list.

`make_available()` also returns true upon a page change.  In that case it'd be a page backtracking
process, which will be handled as described in [Algorithm](/docs/Algorithm.md)

---
### `WorkerBase`'s event loop
This may be the most important explanation about how the WorkerBase works with its thread's event
loop, although it should be fast-running data processing linear code.

I could have left out the Signals and Slots and only use sync primitives, but I did not think
about that when implementing.  I just wrote a second thread with a communication object, just
like I am used to in Qt.

<u>***The magic lies in the protected Signal `redo()`.***</u> It is connected internally upon
creation and provides a way to "take an event loop-round".  Every time an iteration process is
considered to be finished (and it is known, that another one will follow), `iterate()` should
gracefully end in emitting `redo()`.

This fires a queued signal and upon exiting `iterate()`, the thread event loop regains control.
It will now process all pending events.  Most likely the last event will be the `redo()` signal we
just fired, which runs `pre_re_iterate()`.  This is the function doing all the checks and reporting
the current state if it makes sense.  After these obligations, it directly calls `iterate()` again.

So you see, by emitting `redo()` we make sure to process any events pending - which may be pause
requests, or something under Qt's hood.

---
## deriving your `Worker`
The first implementation `AlgoRunner0` may serve as an example.  This is the current implementation
used and it shows, how you really only need to implement `iterate()`:

```cpp
using namespace Algo;

class AlgoRunner0 : public Factory< WorkerBase >::Registrar< AlgoRunner0 >
{
	Q_OBJECT

  public:
	static const QString name_in_factory;
	virtual ~AlgoRunner0() = default;

  public slots:
	void iterate() override;

  protected:
	void iterate_complete();
};
```
The class declaration line shows how to use the factory on one hand: do not derive from `WorkerBase`
directly, because it itself inherits the factory template.  The inherited factory provides the
templated `Registrar`, which itself is derived from `WorkerBase`, again.  This is indeed confusing,
but when understood works perfectly fine.

The factory mix-in expects a static class variable `name_in_factory` as a QString, which is used
for registering.  This string is the identifier and description of your implementation at the same
time, it will be shown as an item in the algorithm selection combo box.

The protected `iterate_complete()` was added to keep the iterate function simple.  I kind of sourced
out a codepath that is taken only once.  It tries to compute a "completed percentage" from chunks
distributed vs. pages filled and emits the final report with these numbers.

So, most likely you can simply copy & paste from the code block above and replace `AlgoRunner0` with
your classname - header file finished ;)

And ... there you go - the implementation part:
```cpp
#include "AlgoRunner0.h"

using namespace Qt::StringLiterals;

const QString AlgoRunner0::name_in_factory = u"basic implementation with dead pages."_s;

void AlgoRunner0::iterate()
{
	++iteration;
	do { // Außen: Backtracking, Innen: Number Selecting
		while ( a_id < avail.count()
				&& pages[ pageOrder[ p_id ] ]._.bytes_left > cur_btsleft_thresh )
			if ( pages[ pageOrder[ p_id ] ]._.bytes_left >= chunks[ avail[ a_id ] ].size )
			{
				if ( take_available() )
					if ( p_id < pageOrder.count() )
						return emit redo(); // Nächste Page -> neue Iteration!
					else return iterate_complete();
			} else ++a_id;
	} while ( !avail.isEmpty() && !make_available() );

	if ( avail.isEmpty() )
	{
		if ( p_id < pageOrder.count() ) emit page_finished( pageOrder[ p_id ] );
		emit final_solution( iteration, cnt_sel, cnt_unsel, 1.0 );
	} else {
		// Wir sind also nicht fertig geworden, keine Pause angefragt -> das heisst das
		// Backtracking hat einen Seitensprung gemacht.
		if ( pageOrder.isEmpty() || p_id >= pageOrder.count() )
			emit final_solution( iteration, cnt_sel, cnt_unsel, avail.isEmpty() );
		else emit redo();
	}
}

void AlgoRunner0::iterate_complete()
{   ...   }
```
---
So all in all you can use these two code blocks to start your own derrivate of `WorkerBase`.  But
how to get it into the application?  Nothing more, just derive correctly ;)

- derive using the CRTP-mix-in `Algo::Factory< Algo::WorkerBase >::Registrar<>`
- declare and define `name_in_factory`
- declare and implement `iterate()`
- if you declare and define a CTor, make sure it's a CTor without parameters, like a default CTor
  - but really, there's no need to...