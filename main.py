import curses
import random
import time
import json
from pathlib import Path

SAVE = Path.home() / ".redline_save.json"
LANES = 3


def load_best():
    try:
        return json.loads(SAVE.read_text()).get("best", 0)
    except (OSError, ValueError):
        return 0


def save_best(best):
    try:
        SAVE.write_text(json.dumps({"best": best}))
    except OSError:
        pass


def put(scr, y, x, text, color=0):
    h, w = scr.getmaxyx()
    if 0 <= y < h and 0 <= x < w:
        try:
            scr.addnstr(y, x, text, max(0, w - x - 1), color)
        except curses.error:
            pass


def panel(scr, y, x, height, width, title, color):
    if height < 2 or width < 3:
        return
    put(scr, y, x, "+" + "-" * (width - 2) + "+", color)
    put(scr, y + 1, x, "|" + (" " + title)[:width - 2].ljust(width - 2) + "|", color)
    for row in range(y + 2, y + height - 1):
        put(scr, row, x, "|" + " " * (width - 2) + "|", color)
    put(scr, y + height - 1, x, "+" + "-" * (width - 2) + "+", color)


def game(scr):
    curses.curs_set(0)
    scr.nodelay(True)
    scr.keypad(True)
    curses.start_color()
    curses.use_default_colors()

    curses.init_pair(1, curses.COLOR_CYAN, -1)
    curses.init_pair(2, curses.COLOR_GREEN, -1)
    curses.init_pair(3, curses.COLOR_YELLOW, -1)
    curses.init_pair(4, curses.COLOR_RED, -1)
    curses.init_pair(5, curses.COLOR_WHITE, -1)
    curses.init_pair(6, curses.COLOR_MAGENTA, -1)

    cyan = curses.color_pair(1) | curses.A_BOLD
    green = curses.color_pair(2) | curses.A_BOLD
    yellow = curses.color_pair(3)
    red = curses.color_pair(4) | curses.A_BOLD
    white = curses.color_pair(5)
    magenta = curses.color_pair(6)

    best = load_best()
    session = 0

    while True:
        lane = 1
        score = 0
        combo = 0
        speed = 100.0
        traffic = []
        last_spawn = time.monotonic()
        last_time = time.monotonic()
        road_phase = 0
        message = "GREEN FLAG - PUSH!"
        message_until = time.monotonic() + 2
        overtakes = 0
        crashes = 0

        while True:
            now = time.monotonic()
            dt = min(now - last_time, 0.05)
            last_time = now
            h, w = scr.getmaxyx()
            scr.erase()

            if h < 24 or w < 76:
                put(scr, 1, 2, "REDLINE MOTORSPORT", cyan)
                put(scr, 3, 2, "Resize terminal to at least 76 columns x 24 rows.")
                put(scr, 5, 2, "Press Q to quit.")
                scr.refresh()
                if scr.getch() in (ord("q"), ord("Q")):
                    return
                time.sleep(0.1)
                continue

            # Input
            key = scr.getch()
            while key != -1:
                if key in (curses.KEY_LEFT, ord("a"), ord("A")):
                    if lane > 0:
                        lane -= 1
                        message = "LANE CHANGE"
                        message_until = now + 0.5
                elif key in (curses.KEY_RIGHT, ord("d"), ord("D")):
                    if lane < 2:
                        lane += 1
                        message = "LANE CHANGE"
                        message_until = now + 0.5
                elif key in (ord("q"), ord("Q")):
                    save_best(best)
                    return
                key = scr.getch()

            speed = min(285, 100 + score / 24)
            road_phase += dt * speed / 12

            # Traffic spawning
            spawn_gap = max(0.32, 0.95 - speed / 500)
            if now - last_spawn >= spawn_gap:
                occupied = {
                    c["lane"] for c in traffic
                    if c["y"] < h - 10
                }
                choices = [i for i in range(3) if i not in occupied]
                if choices:
                    traffic.append({
                        "lane": random.choice(choices),
                        "y": 5.0,
                        "type": random.choice(["[V]", "[M]", "[R]"]),
                        "color": random.choice([white, yellow, magenta]),
                        "near": False
                    })
                last_spawn = now

            road_top = 5
            road_bottom = h - 5
            road_h = road_bottom - road_top
            player_y = road_bottom - 2
            road_left = max(24, (w - 39) // 2)
            road_width = min(39, w - road_left - 24)
            lane_width = road_width // 3

            # Move traffic
            movement = dt * (5 + speed / 18)
            for car in traffic:
                car["y"] += movement

            # Collision and close passes
            crashed = False
            for car in traffic:
                if int(car["y"]) == player_y and car["lane"] == lane:
                    crashed = True
                    crashes += 1
                    break
                if (
                    not car["near"]
                    and int(car["y"]) >= player_y - 1
                    and int(car["y"]) < player_y
                ):
                    car["near"] = True
                    combo += 1
                    overtakes += 1
                    score += 100 * min(combo, 10)
                    message = f"NEAR MISS! +{100 * min(combo, 10)}"
                    message_until = now + 1.5

            traffic = [c for c in traffic if c["y"] < road_bottom]

            # Race score
            if not crashed:
                score += int(dt * speed / 4)
            best = max(best, score)

            # Header
            put(scr, 0, 2, "REDLINE", cyan)
            put(scr, 0, 12, "// MOTORSPORT SIMULATION", white)
            put(scr, 0, w - 27, "LIVE  |  NIGHT RUN", red)
            put(scr, 1, 2, "=" * (w - 4), cyan)

            # Race telemetry panel
            put(scr, 2, 2, f"SCORE {score:08}", yellow)
            put(scr, 2, 22, f"BEST {best:08}", green)
            put(scr, 2, 42, f"COMBO x{min(combo + 1, 10)}", magenta)
            put(scr, 2, 60, f"OVERTAKES {overtakes:03}", white)

            # Side telemetry panels
            panel(scr, 4, 2, 9, 19, "VEHICLE", cyan)
            put(scr, 7, 4, f"{int(speed):03} KM/H", yellow)
            put(scr, 9, 4, f"GEAR {min(6, 1 + int(speed / 50))}", white)
            rpm = 2500 + int(speed * 22) % 7000
            put(scr, 11, 4, f"RPM {rpm:04}", red)

            panel(scr, 14, 2, 7, 19, "CONDITION", green)
            put(scr, 17, 4, "ENGINE  OK", green)
            put(scr, 18, 4, "TYRES   OK", green)

            panel(scr, 4, w - 22, 9, 20, "RACE CONTROL", yellow)
            put(scr, 7, w - 20, "STATUS", white)
            put(scr, 8, w - 20, "RACING", green)
            put(scr, 10, w - 20, "BEST LAP", white)
            put(scr, 11, w - 20, "--:--.---", yellow)

            panel(scr, 14, w - 22, 7, 20, "CONTROLS", magenta)
            put(scr, 17, w - 20, "<- / A LEFT", white)
            put(scr, 18, w - 20, "-> / D RIGHT", white)
            put(scr, 19, w - 20, "Q QUIT", white)

            # Track
            for y in range(road_top, road_bottom):
                put(scr, y, road_left, "|", white)
                put(scr, y, road_left + road_width, "|", white)
                for divider in (1, 2):
                    x = road_left + divider * lane_width
                    if int(y + road_phase) % 3 != 0:
                        put(scr, y, x, ":", white)

                # Roadside lights
                if (y + int(road_phase * 2)) % 5 == 0:
                    put(scr, y, road_left - 2, ".", yellow)
                    put(scr, y, road_left + road_width + 2, ".", yellow)

            # Traffic
            for car in traffic:
                y = int(car["y"])
                x = road_left + car["lane"] * lane_width + lane_width // 2
                if road_top <= y < road_bottom:
                    put(scr, y, x - 2, car["type"], car["color"])

            # Player car
            player_x = road_left + lane * lane_width + lane_width // 2
            put(scr, player_y, player_x - 3, "<A>", green)

            # Engineer message
            put(scr, h - 3, 2, "ENGINEER >", cyan)
            if now < message_until:
                put(scr, h - 3, 14, message, yellow)
            else:
                put(scr, h - 3, 14, "Find a gap. Keep it clean.", white)

            put(scr, h - 1, 2, "REDLINE // PROTOTYPE 2.0", cyan)
            put(scr, h - 1, w - 29, "SKILL > SPEED > SURVIVAL", white)

            scr.refresh()

            if crashed:
                break

            time.sleep(0.025)

        save_best(best)
        scr.nodelay(False)
        scr.erase()
        put(scr, 4, 4, "RACE OVER", red | curses.A_BOLD)
        put(scr, 6, 4, f"FINAL SCORE: {score}", yellow)
        put(scr, 7, 4, f"PERSONAL BEST: {best}", green)
        put(scr, 8, 4, f"NEAR MISSES: {overtakes}", cyan)
        put(scr, 9, 4, f"COLLISIONS: {crashes}", red)
        put(scr, 12, 4, "R - RESTART RACE", white)
        put(scr, 13, 4, "Q - QUIT", white)
        scr.refresh()

        while True:
            key = scr.getch()
            if key in (ord("r"), ord("R")):
                scr.nodelay(True)
                break
            if key in (ord("q"), ord("Q")):
                return


if __name__ == "__main__":
    try:
        curses.wrapper(game)
    except KeyboardInterrupt:
        pass