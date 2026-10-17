import curses
import random
import time
import json
from pathlib import Path

SAVE_FILE = Path.home() / ".redline_save.json"

LANES = 3
ROAD_HEIGHT = 15


def load_best():
    try:
        return json.loads(SAVE_FILE.read_text()).get("best", 0)
    except (OSError, ValueError):
        return 0


def save_best(best):
    try:
        SAVE_FILE.write_text(json.dumps({"best": best}))
    except OSError:
        pass


def center(stdscr, y, text, color=0):
    height, width = stdscr.getmaxyx()
    if 0 <= y < height and width > len(text) + 2:
        x = max(0, (width - len(text)) // 2)
        try:
            stdscr.addstr(y, x, text, color)
        except curses.error:
            pass


def game(stdscr):
    curses.curs_set(0)
    stdscr.nodelay(True)
    stdscr.keypad(True)

    curses.start_color()
    curses.use_default_colors()
    curses.init_pair(1, curses.COLOR_CYAN, -1)
    curses.init_pair(2, curses.COLOR_RED, -1)
    curses.init_pair(3, curses.COLOR_YELLOW, -1)
    curses.init_pair(4, curses.COLOR_GREEN, -1)
    curses.init_pair(5, curses.COLOR_WHITE, -1)

    CYAN = curses.color_pair(1)
    RED = curses.color_pair(2)
    YELLOW = curses.color_pair(3)
    GREEN = curses.color_pair(4)
    WHITE = curses.color_pair(5)

    best = load_best()

    while True:
        lane = 1
        score = 0
        combo = 0
        speed = 0.20
        traffic = []
        particles = []
        last_tick = time.monotonic()
        last_spawn = last_tick
        crashed = False

        while not crashed:
            now = time.monotonic()
            dt = min(now - last_tick, 0.05)
            last_tick = now

            height, width = stdscr.getmaxyx()
            stdscr.erase()

            if height < 22 or width < 52:
                center(stdscr, height // 2,
                       "Enlarge your terminal to at least 52 x 22.")
                stdscr.refresh()
                time.sleep(0.1)
                continue

            road_width = min(37, width - 8)
            lane_width = road_width // LANES
            road_left = (width - road_width) // 2
            road_top = 5
            road_bottom = height - 3

            speed = min(0.20 + score / 1800, 0.85)

            # Input is processed continuously.
            key = stdscr.getch()
            while key != -1:
                if key in (curses.KEY_LEFT, ord("a"), ord("A")):
                    lane = max(0, lane - 1)
                elif key in (curses.KEY_RIGHT, ord("d"), ord("D")):
                    lane = min(LANES - 1, lane + 1)
                elif key in (ord("q"), ord("Q")):
                    return
                key = stdscr.getch()

            # Spawn traffic at intervals that shrink as speed rises.
            if now - last_spawn > max(0.32, 0.95 - speed):
                occupied = {car["lane"] for car in traffic if car["y"] < road_top + 3}
                choices = [i for i in range(LANES) if i not in occupied]
                if choices:
                    traffic.append({
                        "lane": random.choice(choices),
                        "y": float(road_top),
                        "near": False,
                    })
                last_spawn = now

            for car in traffic:
                car["y"] += speed * 15 * dt * 8

            player_y = road_bottom - 2

            # Collision and near-miss scoring.
            for car in traffic:
                if int(car["y"]) == player_y and car["lane"] == lane:
                    crashed = True
                elif (
                    not car["near"]
                    and int(car["y"]) >= player_y - 1
                    and int(car["y"]) < player_y
                ):
                    car["near"] = True
                    combo += 1
                    score += 50 * min(combo, 10)

            traffic = [c for c in traffic if c["y"] < road_bottom]

            # Distance score.
            score += max(1, int(dt * 20 * (1 + combo * 0.08)))
            best = max(best, score)

            # HUD
            center(stdscr, 0, "R E D L I N E", CYAN)
            center(stdscr, 1, "ARCADE // SURVIVE THE RUSH", WHITE)
            center(
                stdscr, 3,
                f"SCORE {score:07}   BEST {best:07}   COMBO x{min(combo + 1, 10)}",
                YELLOW
            )

            # Road edges and lane markers.
            for y in range(road_top, road_bottom):
                if y < height:
                    try:
                        stdscr.addch(y, road_left, curses.ACS_VLINE, WHITE)
                        stdscr.addch(y, road_left + road_width, curses.ACS_VLINE, WHITE)
                        for divider in (1, 2):
                            x = road_left + divider * lane_width
                            if (y + int(now * 12)) % 3 != 0:
                                stdscr.addch(y, x, curses.ACS_VLINE, WHITE)
                    except curses.error:
                        pass

            # Traffic.
            for car in traffic:
                y = int(car["y"])
                x = road_left + car["lane"] * lane_width + lane_width // 2
                if road_top <= y < road_bottom:
                    try:
                        stdscr.addstr(y, x - 2, "[X]", RED | curses.A_BOLD)
                    except curses.error:
                        pass

            # Player car.
            player_x = road_left + lane * lane_width + lane_width // 2
            try:
                stdscr.addstr(player_y, player_x - 2, "[A]", GREEN | curses.A_BOLD)
            except curses.error:
                pass

            center(stdscr, height - 2, "A / LEFT  ◀     ▶  D / RIGHT       Q: QUIT", CYAN)
            stdscr.refresh()
            time.sleep(0.025)

        save_best(best)
        stdscr.nodelay(False)
        stdscr.erase()
        center(stdscr, 5, "CRASH!", RED | curses.A_BOLD)
        center(stdscr, 7, f"FINAL SCORE: {score}", YELLOW)
        center(stdscr, 8, f"PERSONAL BEST: {best}", CYAN)
        center(stdscr, 11, "Press R to race again")
        center(stdscr, 12, "Press Q to quit")
        stdscr.refresh()

        while True:
            key = stdscr.getch()
            if key in (ord("r"), ord("R")):
                stdscr.nodelay(True)
                break
            if key in (ord("q"), ord("Q")):
                return


if __name__ == "__main__":
    try:
        curses.wrapper(game)
    except KeyboardInterrupt:
        pass