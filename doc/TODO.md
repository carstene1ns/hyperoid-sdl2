# NEW TODO LIST:

- Investigate the need for custom randomness functions, nowadays PCG or
  Mersenne Twister or LCG could be used.

- Maybe rewrite the getopt stuff, which seems complicated and bulky.

# 25 Year old TODOs:

- The source is a total mess, mainly because it was horribly Windowsy
  originally and that hasn't really been fixed yet. May even want to
  reindent the source to suit me, something I'd normally be very
  reluctant to do - but I've hacked this *so* much it might be
  justified.

- It may be possible for (completely invisible!) black Spinners to be
  created on level 16 or so. Need to check if that's the case and fix if
  it is. (A proper fix would be a little bit more tricky than it sounds,
  though. The color changes when hit, and is set in (at least) two
  different places.)
