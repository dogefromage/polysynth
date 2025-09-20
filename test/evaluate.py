from matplotlib import pyplot as plt
import pandas as pd

runs = [
    "test/50_tuning_samples_100ms_timespan.csv",
    "test/10_tuning_samples_1000ms_timespan.csv",
]

fig, axs = plt.subplots(len(runs), 2)

for ax_row, run in zip(axs, runs):
    # df = pd.read_csv(, index_col=False)
    df = pd.read_csv(run, index_col=False)

    for ax, mode in zip(ax_row, ["pitch", "cutoff"]):

        ax.set_title(f"{run} ({mode})")

        for voice in range(8):
            pitch_mask = df["type"].str.strip().str.match(mode)
            voice_mask = df["voice"] == voice
            voice_pitch_df = df[pitch_mask & voice_mask]
            x = voice_pitch_df["test_semis"]
            y = voice_pitch_df["actual_semis"]

            ax.plot(x, y, marker="x", markersize=3)
            ax.set_xlabel("played semitones")
            ax.set_ylabel("measured semitones")

fig.legend()

plt.show()
