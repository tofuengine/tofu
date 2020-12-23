# Blitting Optimization

* 15k sprites w/o animation or movement, straight blit calls -> ~52 FPS (release mode)
* disabling transparency and shifting -> 200 FPS (WTF!!!)
  * I was already suspecting that the transparency branches and shifting indexing could affect quite a bit
  * reintroducing just the shifting, FPS down to ~90
  * with only the transparency, FPS down to ~60
  * tweaking the palette index with a bias (constant shifting, i.e. banked palette) -> ~180 FPS
  * transparency w/ banked palette -> ~70 FPS
  * can I get rid of the transparency branch by setting the alpha color and leave it to the GPU?
  