#ifndef _MACHINES_H_
#define _MACHINES_H_

#include "machines-enabled.h"

#ifdef ENABLE_PACMAN  
  #include "machines/pacman/pacman.h"
#endif

#ifdef ENABLE_GALAGA
  #include "machines/galaga/galaga.h"
#endif

#ifdef ENABLE_DKONG
  #include "machines/dkong/dkong.h"
#endif

#ifdef ENABLE_FROGGER
  #include "machines/frogger/frogger.h"
#endif

#ifdef ENABLE_DIGDUG
  #include "machines/digdug/digdug.h"
#endif

#ifdef ENABLE_1942
  #include "machines/1942/1942.h"
#endif

#ifdef ENABLE_EYES
  #include "machines/eyes/eyes.h"
#endif

#ifdef ENABLE_MRTNT
  #include "machines/mrtnt/mrtnt.h"
#endif

#ifdef ENABLE_LIZWIZ
  #include "machines/lizwiz/lizwiz.h"
#endif

#ifdef ENABLE_THEGLOB
  #include "machines/theglob/theglob.h"
#endif

#ifdef ENABLE_CRUSH 
  #include "machines/crush/crush.h"
#endif

#ifdef ENABLE_ANTEATER 
  #include "machines/anteater/anteater.h"
#endif

#ifdef ENABLE_BOMBJACK 
  #include "machines/bombjack/bombjack.h"
#endif

#ifdef ENABLE_MRDO 
  #include "machines/mrdo/mrdo.h"
#endif

#ifdef ENABLE_BAGMAN 
  #include "machines/bagman/bagman.h"
#endif

#ifdef ENABLE_PENGO 
  #include "machines/pengo/pengo.h"
#endif

#ifdef ENABLE_MSPACMAN
  #include "machines/mspacman/mspacman.h"
#endif

#ifdef ENABLE_GALAXIAN
  #include "machines/galaxian/galaxian.h"
#endif

#ifdef ENABLE_LADYBUG
  #include "machines/ladybug/ladybug.h"
#endif

#ifdef ENABLE_SPACEINVADERS
  #include "machines/spaceinvaders/spaceinvaders.h"
#endif

#ifdef ENABLE_TIMEPLT
  #include "machines/timeplt/timeplt.h"
#endif

#ifdef ENABLE_GYRUSS
  #include "machines/gyruss/gyruss.h"
#endif

#ifdef ENABLE_TUTANKHM
  #include "machines/tutankhm/tutankhm.h"
#endif

#ifdef ENABLE_DKONGJR
  #include "machines/dkongjr/dkongjr.h"
#endif

#ifdef ENABLE_STARFORCE
  #include "machines/starforce/starforce.h"
#endif

#ifdef ENABLE_MOONCRESTA
  #include "machines/mooncresta/mooncresta.h"
#endif

#ifdef ENABLE_SCRAMBLE
  #include "machines/scramble/scramble.h"
#endif

#ifdef ENABLE_SUPERCOBRA
  #include "machines/supercobra/supercobra.h"
#endif

#ifdef ENABLE_DKONG3
  #include "machines/dkong3/dkong3.h"
#endif

#ifdef ENABLE_POOYAN
  #include "machines/pooyan/pooyan.h"
#endif

#ifdef ENABLE_PHOENIX
  #include "machines/phoenix/phoenix.h"
#endif

#ifdef ENABLE_BURGERTIME
  #include "machines/burgertime/burgertime.h"
#endif

#ifdef ENABLE_XEVIOUS
  #include "machines/xevious/xevious.h"
#endif

#ifdef ENABLE_BNJ
  #include "machines/bnj/bnj.h"
#endif

#ifdef ENABLE_MAPPY
  #include "machines/mappy/mappy.h"
#endif

#ifdef ENABLE_GAPLUS
  #include "machines/gaplus/gaplus.h"
#endif

#ifdef ENABLE_ALIBABA
  #include "machines/alibaba/alibaba.h"
#endif

#ifdef ENABLE_AMIDAR
  #include "machines/amidar/amidar.h"
#endif

#ifdef ENABLE_TURTLES
  #include "machines/turtles/turtles.h"
#endif

#ifdef ENABLE_CIRCUSC
  #include "machines/circusc/circusc.h"
#endif

#ifdef ENABLE_ROCNROPE
  #include "machines/rocnrope/rocnrope.h"
#endif

// change machine order is possible here...
machineBase *machines[] = {
#ifdef ENABLE_PACMAN  
  new pacman(),
#endif
#ifdef ENABLE_GALAGA  
  new galaga(), 
#endif  
#ifdef ENABLE_DIGDUG  
  new digdug(), 
#endif  
#ifdef ENABLE_FROGGER  
  new frogger(), 
#endif  
#ifdef ENABLE_DKONG  
  new dkong(), 
#endif  
#ifdef ENABLE_1942  
  new _1942(), 
#endif  
#ifdef ENABLE_LIZWIZ  
  new lizwiz(),
#endif  
#ifdef ENABLE_EYES  
  new eyes(), 
#endif  
#ifdef ENABLE_MRTNT  
  new mrtnt(), 
#endif  
#ifdef ENABLE_THEGLOB 
  new theglob(),
#endif
#ifdef ENABLE_CRUSH 
  new crush(),
#endif
#ifdef ENABLE_ANTEATER 
  new anteater(),
#endif
#ifdef ENABLE_BOMBJACK 
  new bombjack(),
#endif
#ifdef ENABLE_MRDO 
  new mrdo(),
#endif
#ifdef ENABLE_BAGMAN 
  new bagman(),
#endif
#ifdef ENABLE_PENGO 
  new pengo(),
#endif
#ifdef ENABLE_MSPACMAN
  new mspacman(),
#endif
#ifdef ENABLE_GALAXIAN
  new galaxian(),
#endif
#ifdef ENABLE_LADYBUG
  new ladybug(),
#endif
#ifdef ENABLE_SPACEINVADERS
  new spaceinvaders(),
#endif
#ifdef ENABLE_TIMEPLT
  new timeplt(),
#endif
#ifdef ENABLE_GYRUSS 
  new gyruss(),
#endif
#ifdef ENABLE_TUTANKHM
  new tutankhm(),
#endif
#ifdef ENABLE_DKONGJR
  new dkongjr(),
#endif
#ifdef ENABLE_STARFORCE
  new starforce(),
#endif
#ifdef ENABLE_MOONCRESTA
  new mooncresta(),
#endif
#ifdef ENABLE_SCRAMBLE
  new scramble(),
#endif
#ifdef ENABLE_SUPERCOBRA
  new supercobra(),
#endif
#ifdef ENABLE_DKONG3
  new dkong3(),
#endif
#ifdef ENABLE_POOYAN
  new pooyan(),
#endif
#ifdef ENABLE_PHOENIX
  new phoenix(),
#endif
#ifdef ENABLE_BURGERTIME
  new burgertime(),
#endif
#ifdef ENABLE_XEVIOUS
  new xevious(),
#endif
#ifdef ENABLE_BNJ
  new bnj(),
#endif
#ifdef ENABLE_MAPPY
  new mappy(),
#endif
#ifdef ENABLE_GAPLUS
  new gaplus(),
#endif
#ifdef ENABLE_ALIBABA
  new alibaba(),
#endif
#ifdef ENABLE_AMIDAR
  new amidar(),
#endif
#ifdef ENABLE_TURTLES
  new turtles(),
#endif
#ifdef ENABLE_CIRCUSC
  new circusc(),
#endif
#ifdef ENABLE_ROCNROPE
  new rocnrope(),
#endif
};

template <std::size_t N, class T>
constexpr std::size_t countof(T(&)[N]) { return N; }
static_assert(countof(machines) >= 1, "At least one machine has to be enabled!");

#endif // _MACHINES_H_
