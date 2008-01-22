/*
 * Copyright (C) 2003 Robert Kooima
 *
 * NEVERBALL is  free software; you can redistribute  it and/or modify
 * it under the  terms of the GNU General  Public License as published
 * by the Free  Software Foundation; either version 2  of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 */

#include "gui.h"
#include "set.h"
#include "util.h"
#include "game.h"
#include "levels.h"
#include "audio.h"
#include "config.h"
#include "st_shared.h"

#include "st_set.h"
#include "st_over.h"
#include "st_level.h"
#include "st_start.h"
#include "st_title.h"

/*---------------------------------------------------------------------------*/

#define START_BACK      -1
#define START_PRACTICE  -2
#define START_NORMAL    -3
#define START_CHALLENGE -4

static int shot_id;
static int status_id;

/*---------------------------------------------------------------------------*/

/* Create a level selector button based upon its existence and status. */

static void gui_level(int id, int i)
{
    const GLfloat *fore, *back;
    const struct level *l;

    int jd;

    if (!set_level_exists(curr_set(), i))
    {
        gui_space(id);
        return;
    }

    l = get_level(i);

    if (!l->is_locked)
    {
        fore = l->is_bonus     ? gui_grn : gui_wht;
        back = l->is_completed ? fore    : gui_yel;
    }
    else
        fore = back = gui_gry;

    jd = gui_label(id, l->repr, GUI_SML, GUI_ALL, back, fore);

    gui_active(jd, i, 0);
}

static void start_over_level(int i)
{
    const struct level *l = get_level(i);
    if (!l->is_locked || config_cheat())
    {
        gui_set_image(shot_id, l->shot);

        set_most_coins(&l->score.most_coins, -1);

        if (curr_mode() == MODE_PRACTICE)
        {
            set_best_times(&l->score.best_times, -1, 0);
            if (l->is_bonus)
                gui_set_label(status_id,
                              _("Play this bonus level in practice mode"));
            else
                gui_set_label(status_id,
                              _("Play this level in practice mode"));
        }
        else
        {
            set_best_times(&l->score.unlock_goal, -1, 1);
            if (l->is_bonus)
                gui_set_label(status_id,
                              _("Play this bonus level in normal mode"));
            else
                gui_set_label(status_id, _("Play this level in normal mode"));
        }
        if (config_cheat())
        {
            gui_set_label(status_id, l->file);
        }
        return;
    }
    else if (l->is_bonus)
        gui_set_label(status_id,
                      _("Play in challenge mode to unlock extra bonus levels"));
    else
        gui_set_label(status_id,
                      _("Finish previous levels to unlock this level"));
}

static void start_over(int id)
{
    int i;

    gui_pulse(id, 1.2f);
    if (id == 0)
        return;

    i = gui_token(id);


    switch (i)
    {
    case START_CHALLENGE:
        gui_set_image(shot_id, set_shot(curr_set()));
        set_most_coins(set_coin_score(curr_set()), -1);
        set_best_times(set_time_score(curr_set()), -1, 0);
        gui_set_label(status_id, _("Challenge all levels from the first one"));
        break;

    case START_NORMAL:
        gui_set_label(status_id, _("Collect coins and unlock next level"));
        break;

    case START_PRACTICE:
        gui_set_label(status_id, _("Train yourself without time nor coin"));
        break;
    }

    if (i >= 0)
        start_over_level(i);
}

/*---------------------------------------------------------------------------*/

static int start_action(int i)
{
    int mode = curr_mode();

    audio_play(AUD_MENU, 1.0f);

    switch (i)
    {
    case START_BACK:
        return goto_state(&st_set);
    case START_NORMAL:
        mode_set(MODE_NORMAL);
        return goto_state(&st_start);
    case START_PRACTICE:
        mode_set(MODE_PRACTICE);
        return goto_state(&st_start);
    }

    if (i == START_CHALLENGE)
    {
        /* On cheat, start challenge mode where you want */
        if (config_cheat())
        {
            mode_set(MODE_CHALLENGE);
            return goto_state(&st_start);
        }
        i = 0;
        mode = MODE_CHALLENGE;
    }

    if (i >= 0)
    {
        const struct level *l = get_level(i);

        if (!l->is_locked || config_cheat())
        {
            if (level_play(l, mode))
            {
                return goto_state(&st_level);
            }
            else
            {
                level_stop();
                return 1;
            }
        }
    }
    return 1;
}

static int start_enter(void)
{
    int w = config_get_d(CONFIG_WIDTH);
    int h = config_get_d(CONFIG_HEIGHT);
    int m = curr_mode();
    int i, j;

    int id, jd, kd, ld;

    /* Deactivate cheat */

    if (m == MODE_CHALLENGE && !config_cheat())
    {
        mode_set(MODE_NORMAL);
        m = MODE_NORMAL;
    }

    if ((id = gui_vstack(0)))
    {
        if ((jd = gui_hstack(id)))
        {

            gui_label(jd, set_name(curr_set()), GUI_SML, GUI_ALL,
                      gui_yel, gui_red);
            gui_filler(jd);
            gui_start(jd, _("Back"),  GUI_SML, START_BACK, 0);
        }


        if ((jd = gui_harray(id)))
        {
            shot_id = gui_image(jd, set_shot(curr_set()), 7 * w / 16, 7 * h / 16);

            if ((kd = gui_varray(jd)))
            {
                if ((ld = gui_harray(kd)))
                {
                    gui_state(ld, _("Practice"), GUI_SML, START_PRACTICE,
                              m == MODE_PRACTICE);
                    gui_state(ld, _("Normal"),   GUI_SML, START_NORMAL,
                              m == MODE_NORMAL);
                }
                for (i = 0; i < 5; i++)
                    if ((ld = gui_harray(kd)))
                        for (j = 4; j >= 0; j--)
                            gui_level(ld, i * 5 + j);

                gui_state(kd, _("Challenge"), GUI_SML, START_CHALLENGE,
                          m == MODE_CHALLENGE);
            }
        }
        gui_space(id);

        if ((jd = gui_harray(id)))
        {
            gui_most_coins(jd, 0);
            gui_best_times(jd, 0);
        }
        gui_space(id);

        status_id = gui_label(id, _("Choose a level to play"), GUI_SML, GUI_ALL,
                              gui_yel, gui_wht);

        gui_layout(id, 0, 0);

        set_most_coins(NULL, -1);
        set_best_times(NULL, -1, m != MODE_PRACTICE);
    }

    audio_music_fade_to(0.5f, "bgm/inter.ogg");

    return id;
}

static void start_point(int id, int x, int y, int dx, int dy)
{
    start_over(gui_point(id, x, y));
}

static void start_stick(int id, int a, int v)
{
    int x = (config_tst_d(CONFIG_JOYSTICK_AXIS_X, a)) ? v : 0;
    int y = (config_tst_d(CONFIG_JOYSTICK_AXIS_Y, a)) ? v : 0;

    start_over(gui_stick(id, x, y));
}

static int start_keybd(int c, int d)
{
    if (d && c == SDLK_c && config_cheat())
    {
        set_cheat();
        return goto_state(&st_start);
    }

    if (d && c == SDLK_F12)
    {
        int i;

        /* Iterate over all levels, taking a screenshot of each. */

        for (i = 0; i < MAXLVL; i++)
            if (set_level_exists(curr_set(), i))
                level_snap(i);
    }

    return 1;
}

static int start_buttn(int b, int d)
{
    if (d)
    {
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_A, b))
            return start_action(gui_token(gui_click()));
        if (config_tst_d(CONFIG_JOYSTICK_BUTTON_EXIT, b))
            return start_action(START_BACK);
    }
    return 1;
}

/*---------------------------------------------------------------------------*/

struct state st_start = {
    start_enter,
    shared_leave,
    shared_paint,
    shared_timer,
    start_point,
    start_stick,
    shared_angle,
    shared_click,
    start_keybd,
    start_buttn,
    1, 0
};