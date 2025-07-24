Chunk Distribution Visualizer - how to collaborate ?
====================================================
As I really found no good idea on which occasion the algorithm should change or f.e. allow some
threshold or even page boundary crossings, I decided to make it easy for anybody, to implement
their own ideas without much effort.

A short dive into the program:
- main.cpp is a simple Qt app starter
- PackerTester.h/cpp contains the whole UI, as well as the ATMDS3-Implementation (that's why it
runs much slower even in `silent` mode - first try without multithreading.)
- Stage.h/cpp contains some QGraphicsWidgets that are used for visualization
- AnimatePosition.h/cpp contains a CRTP-snapin for animated QGraphicsObject movement.

All those code files rather belong to the first implementation (at least that is what makes them
a little crowded), and to application / UI control.

- AlgoData.h
- Algo.h/cpp
- AlgoGfx.h/cpp

Were kind of refactored out them.  I wanted to show off without any need, so I even created a Qt
Designer Plugin for the Algo Widget.  This may be usefull for some preconfiguration if somebody
derives an own algorithm.

## AlgoData.h
... describes data structures:
- `AlgoPage`, which is the base to record the description about a page and at the same time keep
one record to work on and one to present a solution at the same time.
- `ResultCache` - declared as using a `QMap< quin16, AlgoPage>`
- `AlgoCom` - a communication structure for multithreaded working, contains:
  - a ResultCache
  - a mutable ReadWriteLock
  - and a boolean to signalize fresh data

It should be noted that access patterns to this communication structure have to be strictly obeyed.
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

## AlgoGfx.h/cpp
Implements the QGraphicsObjects/Widgets needed to display solutions visually.  It also contains
the declaration of the CRTP-Snapin I use for animating the movement of QGraphicsObjects.

You may want to change stuff there, make it look nicer, whatever - I'd be glad in appreciation.  But for
mathematical purposes it doesn't matter, it's the boilerplate I made so we can *look* at how
different implementations *act*.

## Algo.h/cpp
Here's where the hot stuff happens ;)

First, the Algo-Widget is declared, providing the description, the group
boxes for simulation and generation, the animation group box and the QGraphicsView for visualization.
All Ui is internally interconnected to corresponding slots, so the widget already does its visual
housekeeping by itself.

For collaborating you do not have to care much about UI.  It's about a new `worker` and aforementioned
communication structure `AlgoCom`.  The Algo UI Widget owns such a structure to provide the scratch-
and reporting space.

### AlgoWorker
Algo.h also defines `AlgoWorker`.  This class is pure virtual, as the `iterate()` method has no
base implementation.

This is the class you'd want to derive your algorithm implementation from.  And it really only needs
that single function and a CTor - of course you are free to implement what you think is neccessary.

#### protected Variables:
All variables of `AlgoWorker` are declared protected, thus your derivate can use them just as if it
were your own variables.

| variable(s) | description |
| :--- | :--- |
| `com` | a reference to the communication structure<br/><ul><li/>any worker needs to be constructed with a `&AlgoCom`, so when accessing the memory, it writes to the data structure owned by `Algo`</ul> |
| `a_id` and `p_id` | idices of current available chunk and currently processed page |
| `chunks` | a simple list of integers, representing the chunk sizes given |
| `avail` | sorted list of chunk_ids (i.e. positions inside `chunks`), sorted by chunk size in descending order |
| `pageOrder` | a sorted list of page indices, sorted by free space ascending |
| `badPages` | initially empty, used to rule out pages that simply cannot be filled<ul><li/>in that case, the page_id is removed from `pageOrder` and appended to `badPages`<li/>`p_id` does not change thru this operation, it simply points to the next page (or end) afterwards.|
| `current_state` | I'll go into detail below |
| `cnt_sel`, `cnt_unsel`<td rowspan="2">**some 64bit counters for operations**<ul><li/>count selection/deselection operations (make_available/take_available)<li/>`iteration` ... should be incremented upon entering `iterate()`<li/>`lastIt` ... remembers the last time a progress report had been emitted<ul><li/>Used to constrain progress reports / status changes to ~every 2<sup>14</sup> iterations.<li/>if not done, the Signals & Slots system slows down operations considerably!</ul><li/>`cur_btsleft_thresh` - how many bytes may be left before a page is considered as "filled"?<ul><li/>initially set to zero<li/>I added this lately, because I think this is the direction to go - more below.</ul></ul></td>
| `iteration`, `lastIt` | 
  
