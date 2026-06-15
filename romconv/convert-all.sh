#!/bin/bash

set -e

die() {

  echo "FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL"
  exit 1 
}

#------------------------------------
# z80 
#------------------------------------

# unzip and patch z80 emulator
python3 ./z80patch.py || die
dos2unix ../source/src/cpus/z80/* || die
echo "Z80 done"

#------------------------------------
# pacman
#------------------------------------

if [[ -f ../romszip/pacman.zip ]]; then
  echo Pacman Unpack roms
  python3 ./unpack.py pacman.zip || die 

  #echo Pacman Logos
  #python3 ./logoconv.py ../logos/pacman.png ../source/src/machines/pacman/pacman_logo.h || die

  echo Pacman CPU code
  python3 ./romconv.py pacman_rom ./roms/pacman.6e ./roms/pacman.6f ./roms/pacman.6h ./roms/pacman.6j ../source/src/machines/pacman/pacman_rom.h || die

  echo Pacman Tiles
  python3 ./tileconv.py pacman_tilemap ./roms/pacman.5e ../source/src/machines/pacman/pacman_tilemap.h || die

  echo Pacman Sprites
  python3 ./spriteconv.py pacman_sprites pacman ./roms/pacman.5f ../source/src/machines/pacman/pacman_spritemap.h || die

  echo Pacman Colormaps
  python3 ./cmapconv.py pacman_colormap ./roms/82s123.7f 0 ./roms/82s126.4a ../source/src/machines/pacman/pacman_cmap.h || die

  echo Pacman Audio
  python3 ./audioconv.py pacman_wavetable ./roms/82s126.1m ../source/src/machines/pacman/pacman_wavetable.h || die
fi

#------------------------------------
# Galaga
#------------------------------------

if [[ -f ../romszip/galaga.zip ]]; then
  echo Galaga Unpack roms
  python3 ./unpack.py galaga.zip || die

  #echo Galaga Logos
  #python3 ./logoconv.py ../logos/galaga.png ../source/src/machines/galaga/galaga_logo.h || die

  echo Galaga CPU code
  python3 ./romconv.py -p galaga_rom_cpu1 ./roms/gg1_1b.3p ./roms/gg1_2b.3m ./roms/gg1_3.2m ./roms/gg1_4b.2l ../source/src/machines/galaga/galaga_rom1.h || die
  python3 ./romconv.py galaga_rom_cpu2 ./roms/gg1_5b.3f ../source/src/machines/galaga/galaga_rom2.h || die
  python3 ./romconv.py galaga_rom_cpu3 ./roms/gg1_7b.2c ../source/src/machines/galaga/galaga_rom3.h || die

  echo Galaga Tiles
  python3 ./tileconv.py galaga_tilemap ./roms/gg1_9.4l ../source/src/machines/galaga/galaga_tilemap.h || die

  echo Galaga Sprites
  python3 ./spriteconv.py galaga_sprites galaga ./roms/gg1_11.4d ./roms/gg1_10.4f ../source/src/machines/galaga/galaga_spritemap.h || die

  echo Galaga Colormaps
  python3 ./cmapconv.py galaga_colormap_sprites ./roms/prom-5.5n 0 ./roms/prom-3.1c ../source/src/machines/galaga/galaga_cmap_sprites.h || die
  python3 ./cmapconv.py galaga_colormap_tiles ./roms/prom-5.5n 16 ./roms/prom-4.2n ../source/src/machines/galaga/galaga_cmap_tiles.h || die

  echo Galaga Audio
  python3 ./audioconv.py galaga_wavetable ./roms/prom-1.1d ../source/src/machines/galaga/galaga_wavetable.h || die
fi

#------------------------------------
# Dig Dug
#------------------------------------

if [[ -f ../romszip/digdug.zip ]]; then
  echo Digdug Unpack roms
  python3 ./unpack.py digdug.zip || die

  #echo Digdug Logos
  #python3 ./logoconv.py ../logos/digdug.png ../source/src/machines/digdug/digdug_logo.h || die

  echo Digdug CPU code
  python3 ./romconv.py digdug_rom_cpu1 ./roms/dd1a.1 ./roms/dd1a.2 ./roms/dd1a.3 ./roms/dd1a.4 ../source/src/machines/digdug/digdug_rom1.h || die
  python3 ./romconv.py digdug_rom_cpu2 ./roms/dd1a.5 ./roms/dd1a.6 ../source/src/machines/digdug/digdug_rom2.h || die
  python3 ./romconv.py digdug_rom_cpu3 ./roms/dd1.7 ../source/src/machines/digdug/digdug_rom3.h || die
  python3 ./romconv.py digdug_playfield ./roms/dd1.10b ../source/src/machines/digdug/digdug_playfield.h || die

  echo Digdug Tiles
  python3 ./tileconv.py digdug_tilemap ./roms/dd1.9 ../source/src/machines/digdug/digdug_tilemap.h || die
  python3 ./tileconv.py digdug_pftiles ./roms/dd1.11 ../source/src/machines/digdug/digdug_pftiles.h || die

  echo Digdug Sprites
  python3 ./spriteconv.py digdug_sprites digdug ./roms/dd1.15 ./roms/dd1.14 ./roms/dd1.13 ./roms/dd1.12 ../source/src/machines/digdug/digdug_spritemap.h || die

  echo Digdug Colormaps
  python3 ./cmapconv.py digdug_colormap_tiles ./roms/136007.113 0 ./roms/136007.112 ../source/src/machines/digdug/digdug_cmap_tiles.h || die
  python3 ./cmapconv.py digdug_colormap_sprites ./roms/136007.113 16 ./roms/136007.111 ../source/src/machines/digdug/digdug_cmap_sprites.h || die
  python3 ./cmapconv.py digdug_colormaps ./roms/136007.113 ../source/src/machines/digdug/digdug_cmap.h

  echo Digdug Audio
  python3 ./audioconv.py digdug_wavetable ./roms/136007.110 ../source/src/machines/digdug/digdug_wavetable.h || die
fi

#------------------------------------
# frogger
#------------------------------------

if [[ -f ../romszip/frogger.zip ]]; then
  echo Frogger Unpack roms
  python3 ./unpack.py frogger.zip || die

  #echo Frogger Logos
  #python3 ./logoconv.py ../logos/frogger.png ../source/src/machines/frogger/frogger_logo.h | die

  echo Frogger CPU code
  python3 ./romconv.py frogger_rom_cpu1 ./roms/frogger.26 ./roms/frogger.27 ./roms/frsm3.7 ../source/src/machines/frogger/frogger_rom1.h || die
  python3 ./romconv.py frogger_rom_cpu2 ./roms/frogger.608 ./roms/frogger.609 ./roms/frogger.610 ../source/src/machines/frogger/frogger_rom2.h || die

  echo Frogger Tiles
  python3 ./tileconv.py frogger_tilemap ./roms/frogger.606 ./roms/frogger.607 ../source/src/machines/frogger/frogger_tilemap.h || die

  echo Frogger Sprites
  python3 ./spriteconv.py frogger_sprites frogger ./roms/frogger.606 ./roms/frogger.607 ../source/src/machines/frogger/frogger_spritemap.h || die

  echo Frogger Colormaps
  python3 ./cmapconv.py frogger_colormap ./roms/pr-91.6l ../source/src/machines/frogger/frogger_cmap.h || die
fi

#------------------------------------
# dkong - donkey kong
#------------------------------------

if [[ -f ../romszip/dkong.zip ]]; then
  echo Dkong Unpack roms
  python3 ./unpack.py dkong.zip || die

  #echo Dkong Logos
  #python3 ./logoconv.py ../logos/dkong.png ../source/src/machines/dkong/dkong_logo.h || die

  echo Dkong CPU code
  python3 ./romconv.py dkong_rom_cpu1 ./roms/c_5et_g.bin ./roms/c_5ct_g.bin ./roms/c_5bt_g.bin ./roms/c_5at_g.bin ../source/src/machines/dkong/dkong_rom1.h || die
  python3 ./romconv.py dkong_rom_cpu2 ./roms/s_3i_b.bin ./roms/s_3j_b.bin ../source/src/machines/dkong/dkong_rom2.h || die

  echo Dkong Tiles
  python3 ./tileconv.py dkong_tilemap ./roms/v_5h_b.bin ./roms/v_3pt.bin ../source/src/machines/dkong/dkong_tilemap.h || die

  echo Dkong Sprites
  python3 ./spriteconv.py dkong_sprites dkong ./roms/l_4m_b.bin  ./roms/l_4n_b.bin  ./roms/l_4r_b.bin  ./roms/l_4s_b.bin ../source/src/machines/dkong/dkong_spritemap.h || die

  echo Dkong Colormaps
  python3 ./cmapconv.py dkong_colormap ./roms/c-2k.bpr ./roms/c-2j.bpr 0 ./roms/v-5e.bpr ../source/src/machines/dkong/dkong_cmap.h || die
fi

#------------------------------------
# 1942
#------------------------------------

if [[ -f ../romszip/1942.zip ]]; then
  echo 1942 Unpack roms
  python3 ./unpack.py 1942.zip || die

  #echo 1942 Logos
  #python3 ./logoconv.py ../logos/1942.png ../source/src/machines/1942/1942_logo.h || die

  echo 1942 CPU code
  python3 ./romconv.py _1942_rom_cpu1 ./roms/srb-03.m3 ./roms/srb-04.m4 ../source/src/machines/1942/1942_rom1.h || die
  python3 ./romconv.py _1942_rom_cpu1_b0 ./roms/srb-05.m5 ../source/src/machines/1942/1942_rom1_b0.h || die
  python3 ./romconv.py _1942_rom_cpu1_b1 ./roms/srb-06.m6 ../source/src/machines/1942/1942_rom1_b1.h || die
  python3 ./romconv.py _1942_rom_cpu1_b2 ./roms/srb-07.m7 ../source/src/machines/1942/1942_rom1_b2.h || die
  python3 ./romconv.py _1942_rom_cpu2 ./roms/sr-01.c11 ../source/src/machines/1942/1942_rom2.h || die

  echo 1942 Tiles
  python3 ./tileconv.py _1942_charmap ./roms/sr-02.f2 ../source/src/machines/1942/1942_charmap.h || die
  python3 ./tileconv.py _1942_tilemap ./roms/sr-08.a1 ./roms/sr-09.a2 ./roms/sr-10.a3 ./roms/sr-11.a4 ./roms/sr-12.a5 ./roms/sr-13.a6 ../source/src/machines/1942/1942_tilemap.h || die

  echo 1942 Sprites
  python3 ./spriteconv.py _1942_sprites 1942 ./roms/sr-14.l1 ./roms/sr-15.l2 ./roms/sr-16.n1 ./roms/sr-17.n2 ../source/src/machines/1942/1942_spritemap.h || die

  echo 1942 Colormaps
  python3 ./cmapconv.py _1942_colormap_chars ./roms/sb-5.e8,./roms/sb-6.e9,./roms/sb-7.e10 128 ./roms/sb-0.f1 ../source/src/machines/1942/1942_character_cmap.h || die
  python3 ./cmapconv.py _1942_colormap_tiles ./roms/sb-5.e8,./roms/sb-6.e9,./roms/sb-7.e10 -1 ./roms/sb-4.d6,./roms/sb-3.d2,./roms/sb-2.d1 ../source/src/machines/1942/1942_tile_cmap.h || die
  python3 ./cmapconv.py _1942_colormap_sprites ./roms/sb-5.e8,./roms/sb-6.e9,./roms/sb-7.e10 64 ./roms/sb-8.k3 ../source/src/machines/1942/1942_sprite_cmap.h || die
fi

#------------------------------------
# lizwiz
#------------------------------------

if [[ -f ../romszip/lizwiz.zip ]]; then
  echo Lizwiz Unpack roms
  python3 ./unpack.py lizwiz.zip || die

  #echo Lizwiz Logos
  #python3 ./logoconv.py ../logos/lizwiz.png ../source/src/machines/lizwiz/lizwiz_logo.h || die

  echo Lizwiz CPU code
  python3 ./romconv.py lizwiz_rom ./roms/6e.cpu ./roms/6f.cpu ./roms/6h.cpu ./roms/6j.cpu ./roms/wiza ./roms/wizb ../source/src/machines/lizwiz/lizwiz_rom.h || die

  echo Lizwiz Tiles
  python3 ./tileconv.py lizwiz_tilemap ./roms/5e.cpu ../source/src/machines/lizwiz/lizwiz_tilemap.h || die

  echo Lizwiz Sprites
  python3 ./spriteconv.py lizwiz_sprites lizwiz ./roms/5f.cpu ../source/src/machines/lizwiz/lizwiz_spritemap.h || die

  echo Lizwiz Colormaps
  python3 ./cmapconv.py lizwiz_colormap ./roms/7f.cpu 0 ./roms/4a.cpu ../source/src/machines/lizwiz/lizwiz_cmap.h || die

  echo Lizwiz Audio
  python3 ./audioconv.py lizwiz_wavetable ./roms/82s126.1m ../source/src/machines/lizwiz/lizwiz_wavetable.h || die
fi

#------------------------------------
# Eyes
#------------------------------------

if [[ -f ../romszip/eyes.zip ]]; then
  echo Eyes Unpack roms
  python3 ./unpack.py eyes.zip || die

  #echo Eyes Logos
  #python3 ./logoconv.py ../logos/eyes.png ../source/src/machines/eyes/eyes_logo.h || die

  echo Eyes CPU code
  python3 ./romconv.py -d eyes_rom ./roms/d7 ./roms/e7 ./roms/f7 ./roms/h7 ../source/src/machines/eyes/eyes_rom.h || die

  echo Eyes Tiles
  python3 ./tileconv.py eyes_tilemap ./roms/d5 ../source/src/machines/eyes/eyes_tilemap.h || die

  echo Eyes Sprites
  python3 ./spriteconv.py eyes_sprites eyes ./roms/e5 ../source/src/machines/eyes/eyes_spritemap.h || die

  echo Eyes Colormaps
  python3 ./cmapconv.py eyes_colormap ./roms/82s123.7f 0 ./roms/82s129.4a ../source/src/machines/eyes/eyes_cmap.h || die

  echo Eyes Audio
  python3 ./audioconv.py eyes_wavetable ./roms/82s126.1m ../source/src/machines/eyes/eyes_wavetable.h || die
fi

#------------------------------------
# Mr. TNT
#------------------------------------

if [[ -f ../romszip/mrtnt.zip ]]; then
  echo Mr. TNT Unpack roms
  python3 ./unpack.py mrtnt.zip || die

  #echo Mr. TNT Logos
  #python3 ./logoconv.py ../logos/mrtnt.png ../source/src/machines/mrtnt/mrtnt_logo.h || die

  echo Mr. TNT CPU code
  python3 ./romconv.py -d mrtnt_rom ./roms/tnt.1 ./roms/tnt.2 ./roms/tnt.3 ./roms/tnt.4 ../source/src/machines/mrtnt/mrtnt_rom.h || die
  echo Mr. TNT Tiles
  python3 ./tileconv.py mrtnt_tilemap ./roms/tnt.5 ../source/src/machines/mrtnt/mrtnt_tilemap.h || die

  echo Mr. TNT Sprites
  python3 ./spriteconv.py mrtnt_sprites mrtnt ./roms/tnt.6 ../source/src/machines/mrtnt/mrtnt_spritemap.h || die

  echo Mr. TNT Colormaps
  python3 ./cmapconv.py mrtnt_colormap ./roms/82s123.7f 0 ./roms/82s126.4a ../source/src/machines/mrtnt/mrtnt_cmap.h || die

  echo Mr. TNT Audio
  python3 ./audioconv.py mrtnt_wavetable ./roms/82s126.1m ../source/src/machines/mrtnt/mrtnt_wavetable.h || die
fi

#------------------------------------
# The Glob
#------------------------------------

if [[ -f ../romszip/theglobp.zip ]]; then
  echo The Glob Unpack roms
  python3 ./unpack.py theglobp.zip || die

  #echo The Glob Logos
  #python3 ./logoconv.py ../logos/theglob.png ../source/src/machines/theglob/theglob_logo.h || die

  echo The Glob CPU code
  python3 ./romconv.py theglob_rom ./roms/glob.u2 ./roms/glob.u3 ../source/src/machines/theglob/theglob_rom.h || die

  echo The Glob Tiles
  python3 ./tileconv.py theglob_tilemap ./roms/glob.5e ../source/src/machines/theglob/theglob_tilemap.h || die

  echo The Glob Sprites
  python3 ./spriteconv.py theglob_sprites pacman ./roms/glob.5f ../source/src/machines/theglob/theglob_spritemap.h || die

  echo The Glob Colormaps
  python3 ./cmapconv.py theglob_colormap ./roms/glob.7f 0 ./roms/glob.4a ../source/src/machines/theglob/theglob_cmap.h || die

  echo The Glob Audio
  python3 ./audioconv.py theglob_wavetable ./roms/82s126.1m ../source/src/machines/theglob/theglob_wavetable.h || die
fi

#------------------------------------
# Crush Roller
#------------------------------------

if [[ -f ../romszip/crush.zip ]]; then
  echo Crush Roller Unpack roms
  python3 ./unpack.py crush.zip || die

  #echo Crush Logos
  #python3 ./logoconv.py ../logos/crush.png ../source/src/machines/crush/crush_logo.h || die

  echo Crush Roller CPU code
  python3 ./romconv.py crush_rom ./roms/crushkrl.6e ./roms/crushkrl.6f ./roms/crushkrl.6h ./roms/crushkrl.6j ../source/src/machines/crush/crush_rom.h || die

  echo Crush Roller Tiles
  python3 ./tileconv.py crush_tilemap ./roms/maketrax.5e ../source/src/machines/crush/crush_tilemap.h || die

  echo Crush Roller Sprites
  python3 ./spriteconv.py crush_sprites crush ./roms/maketrax.5f ../source/src/machines/crush/crush_spritemap.h || die

  echo Crush Roller Colormaps
  python3 ./cmapconv.py crush_colormap ./roms/82s123.7f 0 ./roms/2s140.4a ../source/src/machines/crush/crush_cmap.h || die

  echo Crush Roller Audio
  python3 ./audioconv.py crush_wavetable ./roms/82s126.1m ../source/src/machines/crush/crush_wavetable.h || die
fi

#------------------------------------
# Anteater
#------------------------------------

if [[ -f ../romszip/anteater.zip ]]; then
  echo Anteater Unpack roms
  python3 ./unpack.py anteater.zip || die

  #echo Anteater Logos
  #python3 ./logoconv.py ../logos/anteater.png ../source/src/machines/anteater/anteater_logo.h || die

  echo Anteater CPU code
  python3 ./romconv.py anteater_rom_cpu1 ./roms/ra1-2c ./roms/ra1-2e ./roms/ra1-2f ./roms/ra1-2h ../source/src/machines/anteater/anteater_rom1.h || die
  python3 ./romconv.py anteater_rom_cpu2 ./roms/ra4-5c ./roms/ra4-5d ../source/src/machines/anteater/anteater_rom2.h || die

  echo Anteater Tiles
  python3 ./tileconv.py anteater anteater_tilemap ./roms/ra6-5f ./roms/ra6-5h ../source/src/machines/anteater/anteater_tilemap.h || die

  echo Anteater Sprites
  python3 ./spriteconv.py anteater_sprites anteater ./roms/ra6-5f ./roms/ra6-5h ../source/src/machines/anteater/anteater_spritemap.h || die

  echo Anteater Colormaps
  python3 ./cmapconv.py anteater_colormap ./roms/colr6f.cpu ../source/src/machines/anteater/anteater_cmap.h || die
fi

#------------------------------------
# Bagman
#------------------------------------

if [[ -f ../romszip/bagmanm2.zip ]]; then
  echo Bagman Unpack roms
  python3 ./unpack.py bagmanm2.zip || die

  #echo Bagman Logos
  #python3 ./logoconv.py ../logos/bagman.png ../source/src/machines/bagman/bagman_logo.h || die

  echo Bagman CPU code
  python3 ./romconv.py bagman_rom_cpu ./roms/bagmanm2.1 ./roms/bagmanm2.2 ./roms/bagmanm2.3  ../source/src/machines/bagman/bagman_rom.h || die

  echo Bagman Tiles
  python3 ./tileconv.py bagman bagman_tilemap ./roms/bagmanm2.9 ./roms/bagmanm2.7 ../source/src/machines/bagman/bagman_tilemap.h || die

  echo Bagman Sprites
  python3 ./spriteconv.py bagman_sprites bagman ./roms/bagmanm2.9 ./roms/bagmanm2.7 ../source/src/machines/bagman/bagman_spritemap.h || die

  echo Bagman Colormaps
  python3 ./cmapconv.py bagman_colormap ./roms/bagmanmc.clr ../source/src/machines/bagman/bagman_cmap.h || die
fi

#------------------------------------
# Mr Do!
#------------------------------------

if [[ -f ../romszip/mrdo.zip ]]; then
  echo MrDo Unpack roms
  python3 ./unpack.py mrdo.zip || die

  #echo MrDo Logos
  #python3 ./logoconv.py ../logos/mrdo.png ../source/src/machines/mrdo/mrdo_logo.h || die

  cd mrdo || die

  echo MrDo CPU code
  python3 ./cpu_conv.py || die

  echo MrDo Tiles
  python3 ./bg_tiles.py || die
  python3 ./fg_tiles.py || die

  echo MrDo Sprites
  python3 ./Sprites.py || die

  echo MrDo Colormaps
  python3 ./Palette_mrdo.py || die
  python3 ./sprite_colormap.py || die

  cd ..
fi

#------------------------------------
# Pengo
#------------------------------------

if [[ -f ../romszip/pengo.zip ]] || [[ -f ../romszip/pengoj.zip ]] || [[ -f ../romszip/pengo2u.zip ]]; then
  #echo Pengo Logos
  #python3 ./logoconv.py ../logos/pengo.png ../source/src/machines/pengo/pengo_logo.h || die

  cd pengo || die

  echo Pengo conversion
  python3 ./pengo_rom_convert.py || die

  cd ..
fi

if false && [[ -f ../romszip/pengo2u.zip ]]; then
  echo Pengo Unpack roms
  python3 ./unpack.py pengo2u.zip || die

  #echo Pengo Logos
  #python3 ./logoconv.py ../logos/pengo.png ../source/src/machines/pengo/pengo_logo.h || die

  cd pengo || die

  echo Pengo CPU code
  python3 ./cpu_conv.py || die
  python3 ./audio_conv.py || die

  echo Pengo Tiles
  python3 ./tiles_fg_conv.py || die

  echo Pengo Sprites
  python3 ./sprites_conv.py || die

  echo Pengo Colormaps
  python3 ./colormap.py || die

  cd ..
fi

#------------------------------------
# Bombjack
#------------------------------------

if [[ -f ../romszip/bombjack.zip ]]; then
  echo Bombjack Unpack roms
  python3 ./unpack.py bombjack.zip || die

  #echo Bombjack Logos
  #python3 ./logoconv.py ../logos/bombjack.png ../source/src/machines/bombjack/bombjack_logo.h || die

  cd bombjack || die

  echo Bombjack CPU code
  python3 ./cpu_conv.py || die
  python3 ./audio_cpu_conv.py || die

  echo Bombjack Tiles
  python3 ./tiles_bg_conv.py || die
  python3 ./tiles_fg_conv.py || die
  python3 ./bgmaps_conv.py || die

  echo Bombjack Sprites
  python3 ./sprites_conv.py || die

  cd ..
fi

#------------------------------------
# Ms. Pacman
#------------------------------------

if [[ -f ../romszip/mspacman.zip ]]; then
  echo Ms. Pacman Unpack roms
  python3 ./unpack.py mspacman.zip || die

  #echo Ms. Pacman Logos
  #python3 ./logoconv.py ../logos/mspacman.png ../source/src/machines/mspacman/mspacman_logo.h || die

  echo Converting Ms. Pacman
  cd mspacman
  python3 ./mspacman_rom_convert.py || die
  cd ..
fi

#------------------------------------
# Galaxian
#------------------------------------

if [[ -f ../romszip/galaxian.zip ]]; then
  echo Galaxian Unpack roms
  python3 ./unpack.py galaxian.zip || die
  
  #echo Galaxian Logos
  #python3 ./logoconv.py ../logos/galaxian.png ../source/src/machines/galaxian/galaxian_logo.h || die

  echo Converting Galaxian
  cd galaxian
  python3 ./galaxian_rom_convert.py || die
  cd ..
fi

#------------------------------------
# Lady Bug
#------------------------------------

if [[ -f ../romszip/ladybug.zip ]]; then
  echo Ladybug Unpack roms
  python3 ./unpack.py ladybug.zip || die

  #echo Ladybug Logos
  #python3 ./logoconv.py ../logos/ladybug.png ../source/src/machines/ladybug/ladybug_logo.h || die

  cd ladybug
  python3 ./ladybug_rom_convert.py || die
  cd ..
fi

#------------------------------------
# Time Pilot
#------------------------------------

if [[ -f ../romszip/timeplt.zip ]]; then
  echo Time Pilot Unpack roms
  python3 ./unpack.py timeplt.zip || die

  #echo Time Pilot Logos
  #python3 ./logoconv.py ../logos/timeplt.png ../source/src/machines/timeplt/timeplt_logo.h || die

  echo Converting Time Pilot
  cd timeplt
  python3 ./timeplt_rom_convert.py || die
  cd ..
fi

#------------------------------------
# Space Invaders
#------------------------------------

if [[ -f ../romszip/invaders.zip ]]; then
  echo Space Invaders Unpack roms
  python3 ./unpack.py invaders.zip || die

  #echo Invaders Logos
  #python3 ./logoconv.py ../logos/invaders.png ../source/src/machines/invaders/invaders_logo.h || die

  cd invaders
  python3 ./invaders_rom_convert.py || die
  cd ..
fi

#------------------------------------
# Gyruss
#------------------------------------

if [[ -f ../romszip/gyruss.zip ]]; then
  echo Gyruss Unpack roms
  python3 ./unpack.py gyruss.zip || die

  #echo Gyruss Logos
  #python3 ./logoconv.py ../logos/gyruss.png ../source/src/machines/gyruss/gyruss_logo.h || die

  echo Converting Gyruss
  cd gyruss
  python3 ./gyruss_rom_convert.py || die
  cd ..
fi

#------------------------------------
# Tutankham
#------------------------------------

if [[ -f ../romszip/tutankhm.zip ]]; then
  echo Tutankham Unpack roms
  python3 ./unpack.py tutankhm.zip || die

  #echo Tutankham Logos
  #python3 ./logoconv.py ../logos/tutankhm.png ../source/src/machines/tutankhm/tutankhm_logo.h || die

  echo Converting Tutankham
  cd tutankhm
  python3 tutankhm_rom_convert.py || die
  cd ..
fi

#------------------------------------
# Donkey Kong Jr.
#------------------------------------

if [[ -f ../romszip/dkongjrj.zip ]]; then
  echo Donkey Kong Junior Unpack roms
  python3 ./unpack.py dkongjrj.zip || die

  #echo Donkey Kong Junior Logos
  #python3 ./logoconv.py ../logos/dkongjr.png ../source/src/machines/dkongjr/dkongjr_logo.h || die

  cd dkongjr
  
  echo Donkey Kong Junior CPU code
  python3 ./cpu_conv.py   || die
  python3 ./sound_conv.py || die

  echo Donkey Kong Junior Tiles
  python3 ./tilemap_conv.py       || die
  python3 ./view_tiles_graphic.py || die

  echo Donkey Kong Junior Colormaps
  python3 ./cmap_conv.py || die

  echo Donkey Kong Junior Sprites
  python3 ./sprites_conv.py || die

  cd ..
fi

#------------------------------------
# Star Force
#------------------------------------

if [[ -f ../romszip/starforc.zip ]]; then
  echo --------- Convert Star Force ---------
  echo Star Force Unpack roms
  python3 ./unpack.py starforc.zip || die

  #echo Star Force Logos
  #python3 ./logoconv.py ../logos/starforce.png ../source/src/machines/starforce/starforce_logo.h || die

  cd starforce || die

  echo Star Force CPU code
  python3 ./cpu_conv.py || die

  echo Star Force Tiles
  python3 ./bg1_tiles.py || die
  python3 ./bg2_tiles.py || die
  python3 ./bg3_tiles.py || die
  python3 ./fg_tiles.py  || die

  echo Star Force Sprites
  python3 ./sprites.py || die

  echo Star Force Palette RBG565
  python3 ./starforce_palette.py || die

  cd ..
  
  echo --- Success ---
fi

#------------------------------------
# Moon Cresta
#------------------------------------

if [[ -f ../romszip/mooncrst.zip ]]; then
  echo --------- Convert Star Force ---------
  #echo Moon Cresta Logos
  #python3 ./logoconv.py ../logos/mooncresta.png ../source/src/machines/mooncresta/mooncresta_logo.h || die

  echo Moon Cresta Unpack roms
  python3 ./unpack.py mooncrst.zip || die

  echo Converting Moon Cresta
  cd mooncresta || die
  python3 ./mooncresta_rom_convert.py || die
  cd ..

  echo --- Success ---
fi

#------------------------------------
# Scramble
#------------------------------------

if [[ -f ../romszip/scramble.zip ]]; then
  echo --------- Convert Scramble ---------
  #echo Scramble Logos
  #python3 ./logoconv.py ../logos/scramble.png ../source/src/machines/scramble/scramble_logo.h || die

  echo Converting Scramble
  cd scramble || die
  python3 ./scramble_rom_convert.py || die
  cd ..

  echo --- Success ---
fi

echo -- -------------------------------------------------
echo -- END
echo -- -------------------------------------------------

