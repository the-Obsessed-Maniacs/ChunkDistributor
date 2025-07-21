# <u>Chunk Distributor - the chunk distribution visualizer</u>
<!--TOC-->
  - [About the project / idea](#about-the-project-idea)
  - [About this tool](#about-this-tool)
- [Request for comments, discussion and collaboration](#request-for-comments-discussion-and-collaboration)
- [Further reading / more docs](#further-reading-more-docs)
<!--/TOC-->
## About the project / idea
Originally we @Neoplasia talked about automatic distribution of data chunks in memory in a way, that no page boundaries are crossed in the early 2ks.  At that time the problem at hand was that packed acidtracker music many times didn't sound like in the editor.

While I myself at first only saw the use case for finally creating a really working music packer, discussion quickly made clear that such a tool would be a great addition to C64 coding all around.

Just imagine - some Demo code methods simply need specific short pieces of code at fixed memory locations.  If memory then gets tight, it'd be a tedious job to place some of the tables your code needs, or even small other code fragments in the free space of those "method-code-clobbered" pages.

Maybe democoding isn't the only use case.  When creating editors or games it could help in data management aswell.

## About this tool
Using mostly my old music data and different variations of the old player I found that "one byte can make a big difference".  Most music data and player type combinations took a blink of an eye to calculate a perfect solution - last player page properly filled and all data chunks nicely distributed without page boundary crossings and even without wasting any bytes.

But I also found combinations that seemed to not work out at all.  It calculated and calculated, and calculated, like in a deadloop.

I am a visual person, not good in abstract math, so my way was to visualize what my algorithm does.  And, well, this leads us here ;)

# Request for comments, discussion and collaboration
Before explaining what this tool actually does, I'd like to take the time to ask all fellow readers for help.

Those cases not finding an end to the calculations clearly need further bounds/borders, an abort condition.  Depending on the number of chunks and the number of pages we quickly hit billions of iterations not making any more progress.

Due to the lack of - or myself just not being intelligent enought to find - a simple abort
 condition, I also have not yet added a priority or penalty system, which are planned
 to help deciding where it may be OK to cross a boundary, or simply waste some bytes, not
 filling a page to the brim.

F.e. regarding music data:
- init data being accessed only once - may aswell span a page boundary (lowest priority).
- if the end of a track's bytes reach over a page boundary, it is not that much of a disturbance to the timing (medium priority, data accessed every ~5-10s).
- any instrument table or any of the other tables that may be accessed up to thrice every player call should better not span page boundaries (high priority).

Anyhow - that is the request: <u>**please help me improve the algorithm and find a good abort condition**</u>

# Further reading / more docs
Please have a look at the docs folder:
- [Algorithm explanation / current state / further ideas](/docs/Algorithm.md)
- [User Interface explanation](/docs/UserInterface.md)
- [how to collaborate / use the existing classes to produce own implementations](/docs/Collaborate.md)

