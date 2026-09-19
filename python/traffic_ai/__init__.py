"""Neural traffic drivers: imitation learning, PPO fine-tuning, export.

Everything here works on the 8 "sense" values the engine's traffic AI
sees (src/services/interfaces/workflow/gta5/traffic/gta5_traffic_ai.hpp)
and the 3 controls it returns. Change the layout in one place, change
it in both.
"""
