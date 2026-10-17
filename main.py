
import os
import random

def play():
    lane = 1
    score = 0
    road = [[False, False, False] for _ in range(7)]

    while True:
        os.system("clear")
        print("=" * 35)
        print("          R E D L I N E")
        print("=" * 35)
        print(f"SCORE: {score}")
        print("A = LEFT | D = RIGHT | ENTER = STAY | Q = QUIT\n")

        road.pop(0)
        row = [False, False, False]
        if random.random() < min(0.3 + score / 2000, 0.8):
            row[random.randrange(3)] = True
        road.append(row)

        for i, traffic in enumerate(road):
            cells = []
            for j in range(3):
                if i == 6 and j == lane:
                    cells.append(" YOU ")
                elif traffic[j]:
                    cells.append(" CAR ")
                else:
                    cells.append("  .  ")
            print(" | ".join(cells))

        if road[-1][lane]:
            print("\nCRASH! Race over.")
            print(f"Score: {score}")
            if input("Race again? (y/n): ").lower() != "y":
                break
            lane, score = 1, 0
            road = [[False, False, False] for _ in range(7)]
            continue

        move = input("\nSTEER > ").lower().strip()
        if move == "q":
            break
        if move == "a":
            lane = max(0, lane - 1)
        elif move == "d":
            lane = min(2, lane + 1)

        score += 10

    print("Thanks for playing REDLINE!")

if __name__ == "__main__":
    try:
        play()
    except (KeyboardInterrupt, EOFError):
        print("\nRace ended.")
