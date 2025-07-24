# Chunk Distribution Visualizer - UI explanations
All in all I believe the user interface is kind of self-explaining.  Anyhow, the creator of a thing
always thinks that, but mostly it isn't true.

The tool starts up like this, in the [`Page Filler Algorithmus` mode](#Mode-Page-Filler-Algorithm)
![UI screenshot after start](/docs/pix/sc_startup.png)

On the very left you have the 2 (currently) basic operation modes.  `Page Filler Algorithmus` and
[`ATMDS3 Music Packer`](#Mode-ATMDS3-Music-Packer) - default is page filler.

---
## Mode: Page Filler Algorithm
In the top row you find a button to show an integrated documentation page.  I just happened to put
it there thinking, that way I could document any new findings directly.  But at the end it just
serves as some kind of "inline help" or explanation.

The next row presents 3 distinct configuration sections: [`Example-Data`](#Example-Data), [`Auto-Generator`](#Auto-Generator), and
[`Animation`](#Animation).  These just serve as the names suggest.  Example data drives the simulation.  The
Auto-Generator generates example data as an easy option to feed the algorithm.  And the animation
section lets you select how much you want to visually follow the simulation.

At the bottom you see an empty area - this is the visualization area, used to show current progress and final results.

---
### Auto-Generator
On one hand you are free to produce example data "by hand" by simply typing it into the obvious
fields ([see below](#Example-Data)).  On the other hand you may just click "Generate!" to generate
a data set.  The original bounding values may be a bit too large in some cases.  They just looked
to me like good trade-offs.

- Range of sizes
  > sets the minimum and maximum chunk sizes the generator shall generate
- Range of chunk count
  > sets the range of how many chunks shall be produced
- first Block
  > lowest block base address to generate
- Range of offsets
  > minimum and maximum low address of a page 
- Range of block count
  > min and max amount of blocks to generate
- Extra space (pages) - not used.

---
### Example-Data
Data for the algorithm are chunks on one hand.  We need to know a chunk's size, nothing else.

For that matter the text field `Data-Chunks (sizes)` will be intepreted as a comma-separated list
of byte counts.  The Auto-Generator will generate such a list upon pressing the `Generate!` button.

On the other hand we need "space" to distribute our chunks to.  Those are `Memory Blocks`.

The whole idea of this is targeting C64 / oldschool assembler programming.  Thus the field will be
interpreted as hexadecimal start addresses (16 bit).  If a memory block shall not span the whole
page, the address may be followed by a colon and the number of bytes to fill.  I.e. "$080d:e2"
means:
- the memory page will be @ 0x0800,
- starting free space @ 0x080d, for 0xe2 bytes
- so "something else" will be located starting @ 0x08ef, this space may not be touched.

Lastly, there is the `Simulate!`-Button.  Press it to start the algorithm.  While the algorithm is
running, it will change according to current state, to be useable for pausing the calculations.
(Actually, I guess I added "pausing" because I could and it seemed like a good idea.  But without
a single-step- oder single-iteration-mode, it doesn't make much sense. Atm. I'd see more sense in
an "abort" button.)

---
### Animation
The animation group box is simply for setting animation properties - like if any visual feedback is
wanted at all, if it shall be animated, and how long shall animations take?  It only makes a
difference as long as the algorithm ist running.  Changes should show immediate effect.

---
## Mode: ATMDS3 Music Packer
This was the very first part I implemented to see how my first implementation worked with the music
data.
![screenshot after finishing Confusion](/docs/pix/sc_mp.png)
I'm sorry, it was made in German language and I didn't bother translating.

It is there to have some real-world data to experiment with the very basic [algorithm](/docs/Algorithm.md).

I invite you to play around.  Just run the default setup, it will take 356 iterations.

Maybe select the Tune `Insanity`, change the start address to $186f and let it run in visual
mode. It won't take long, only 182 iterations to find a perfect solution.

Now I invite you to re-select `Insanity` and keep the start address @ $186e - you'll want to switch
thru the different animation modes to see how the brute force algorithm simply tries to find all
possible solutions.  I think it took over 250 million iterations to find no perfect solution, just
tried letting it run, once...
