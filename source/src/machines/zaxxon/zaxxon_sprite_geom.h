#ifndef ZAXXON_SPRITE_GEOM_H
#define ZAXXON_SPRITE_GEOM_H

#include <stdint.h>

// Standalone (no machineBase/Arduino deps) so host_test can exercise it
// directly -- see host_test/test_zaxxon_sprite_wrap.cpp.
//
// MAME's zaxxon_state::draw_sprites (zaxxon_v.cpp) draws every sprite 4x,
// at (sx,sy), (sx,sy-0x100), (sx-0x100,sy) and (sx-0x100,sy-0x100): sx/sy
// come from find_minimum_x/y, which decode a hardware position comparator
// that wraps negative positions into 0-255 via 8-bit arithmetic, so a
// sprite just off one edge can come back with a position near 255 instead
// of near 0. Only the wrapped copy overlaps the visible cliprect in that
// case; skipping it makes the sprite invisible until it drifts far enough
// that the *unwrapped* position lands in range -- it then pops in already
// fully on-screen instead of sliding in from the edge.
//
// zaxxon.cpp's blit_sprite already tries both {0,-256} copies for the band
// axis (spr.y / MAME sx, see BAND_WRAP_OFFSETS). This resolves the other
// axis (spr.x / MAME sy, the dst_x axis after ROT90) the same way.
inline int16_t zaxxon_resolve_sprite_dst_x(uint8_t spr_x, uint8_t oy,
                                            int16_t screen_width) {
  static const int16_t DST_WRAP_OFFSETS[2] = {0, -256};

  for (uint8_t w = 0; w < 2; w++) {
    const int16_t dst_x = 239 - ((int16_t)spr_x + DST_WRAP_OFFSETS[w] + oy);
    if (dst_x >= 0 && dst_x < screen_width) {
      return dst_x;
    }
  }
  return -1;
}

#endif
