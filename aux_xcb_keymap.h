#ifndef AUX_XLIB_KEYMAP_H
#define AUX_XLIB_KEYMAP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <xcb/xcb_keysyms.h>

#define AUX_XCB_KEYMAP_NO_SYM    XCB_NO_SYMBOL

#define AUX_X11_KEYMAP_BS  8
#define AUX_X11_KEYMAP_VT  11
#define AUX_X11_KEYMAP_CR  13
#define AUX_X11_KEYMAP_ESC 27
#define AUX_X11_KEYMAP_DEL 127

/* LATIN1 */
/* XK_space       0x20(DEC 32)  */
/* XK_asciitilde  0x7E(DEC 126) */

/* extended */
#define AUX_X11_KEYMAP_EXT 128

/* F1-F12 */
#define AUX_X11_KEYMAP_F1  (AUX_X11_KEYMAP_EXT + 1)
#define AUX_X11_KEYMAP_F2  (AUX_X11_KEYMAP_EXT + 2)
#define AUX_X11_KEYMAP_F3  (AUX_X11_KEYMAP_EXT + 3)
#define AUX_X11_KEYMAP_F4  (AUX_X11_KEYMAP_EXT + 4)
#define AUX_X11_KEYMAP_F5  (AUX_X11_KEYMAP_EXT + 5)
#define AUX_X11_KEYMAP_F6  (AUX_X11_KEYMAP_EXT + 6)
#define AUX_X11_KEYMAP_F7  (AUX_X11_KEYMAP_EXT + 7)
#define AUX_X11_KEYMAP_F8  (AUX_X11_KEYMAP_EXT + 8)
#define AUX_X11_KEYMAP_F9  (AUX_X11_KEYMAP_EXT + 9)
#define AUX_X11_KEYMAP_F10 (AUX_X11_KEYMAP_EXT + 10)
#define AUX_X11_KEYMAP_F11 (AUX_X11_KEYMAP_EXT + 11)
#define AUX_X11_KEYMAP_F12 (AUX_X11_KEYMAP_EXT + 12)

/* "system" keys */
#define AUX_X11_KEYMAP_PAUSE (AUX_X11_KEYMAP_EXT + 13)
#define AUX_X11_KEYMAP_SCRLL (AUX_X11_KEYMAP_EXT + 14)
#define AUX_X11_KEYMAP_SYSRQ (AUX_X11_KEYMAP_EXT + 15)

/* "grey" keys */
#define AUX_X11_KEYMAP_HOME (AUX_X11_KEYMAP_EXT + 16)
#define AUX_X11_KEYMAP_END  (AUX_X11_KEYMAP_EXT + 17)
#define AUX_X11_KEYMAP_PGUP (AUX_X11_KEYMAP_EXT + 18)
#define AUX_X11_KEYMAP_PGDN (AUX_X11_KEYMAP_EXT + 19)

/* arrow keys */
#define AUX_X11_KEYMAP_LEFT   (AUX_X11_KEYMAP_EXT + 20)
#define AUX_X11_KEYMAP_RIGHT  (AUX_X11_KEYMAP_EXT + 21)
#define AUX_X11_KEYMAP_UP     (AUX_X11_KEYMAP_EXT + 22)
#define AUX_X11_KEYMAP_DOWN   (AUX_X11_KEYMAP_EXT + 23)

/* CTRL & ALT & SHIFT & MENU & INSERT & NUM_LOCK & CAPS LOCK */
#define AUX_X11_KEYMAP_SHIFT_L  (AUX_X11_KEYMAP_EXT + 24)
#define AUX_X11_KEYMAP_SHIFT_R  (AUX_X11_KEYMAP_EXT + 25)
#define AUX_X11_KEYMAP_ALT_L    (AUX_X11_KEYMAP_EXT + 26)
#define AUX_X11_KEYMAP_ALT_R    (AUX_X11_KEYMAP_EXT + 27)
#define AUX_X11_KEYMAP_CTRL_L   (AUX_X11_KEYMAP_EXT + 28)
#define AUX_X11_KEYMAP_CTRL_R   (AUX_X11_KEYMAP_EXT + 29)
#define AUX_X11_KEYMAP_MENU     (AUX_X11_KEYMAP_EXT + 30)
#define AUX_X11_KEYMAP_INS      (AUX_X11_KEYMAP_EXT + 31)
#define AUX_X11_KEYMAP_NUMLOCK  (AUX_X11_KEYMAP_EXT + 32)
#define AUX_X11_KEYMAP_CAPSLOCK (AUX_X11_KEYMAP_EXT + 33)


/* "super" keys */
#define AUX_X11_KEYMAP_SUP_L   (AUX_X11_KEYMAP_EXT + 34)
#define AUX_X11_KEYMAP_SUP_R   (AUX_X11_KEYMAP_EXT + 35)

/* key modifiers: ALT, CTRL, SHIFT */
#define AUX_XCB_KEYMAP_MOD_SHFT     XCB_MOD_MASK_SHIFT
#define AUX_XCB_KEYMAP_MOD_LOCK     XCB_MOD_MASK_LOCK
#define AUX_XCB_KEYMAP_MOD_CTRL     XCB_MOD_MASK_CONTROL
#define AUX_XCB_KEYMAP_MOD_1        XCB_MOD_MASK_1
#define AUX_XCB_KEYMAP_MOD_2        XCB_MOD_MASK_2
#define AUX_XCB_KEYMAP_MOD_3        XCB_MOD_MASK_3
#define AUX_XCB_KEYMAP_MOD_4        XCB_MOD_MASK_4
#define AUX_XCB_KEYMAP_MOD_5        XCB_MOD_MASK_5
#define AUX_XCB_KEYMAP_MOD_ANY      XCB_MOD_MASK_ANY

int aux_xcb_map_keycode(void           *xcb_keysyms,
                        xcb_keycode_t   code, 
                        uint16_t        state, 
						uint32_t       *aux_key, 
                        uint16_t       *aux_key_state);

#ifdef __cplusplus
}
#endif

#endif
