SET(SOURCES screens/ui_Screen1.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    ui_events.c
    images/ui_img_21505875.c
    images/ui_img_1219711373.c
    images/ui_img_70400119.c
    images/ui_img_1905141978.c
    images/ui_img_1907056666.c
    images/ui_img_2091611716.c
    images/ui_img_840545791.c
    images/ui_img_723305328.c
    fonts/ui_font_fontclock48.c
    fonts/ui_font_fontclock24.c
    fonts/ui_font_font16.c
    fonts/ui_font_chinafont16.c
    fonts/ui_font_chinafont32.c
    fonts/ui_font_chinafont20.c)

add_library(ui ${SOURCES})
