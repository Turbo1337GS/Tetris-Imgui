# Tetris Game using ImGui and SFML Audio

This repository contains a Tetris game implemented in C++ using the ImGui library for the user interface and SFML Audio for sound management. The game is played on a grid where tetrominoes fall from the top, and the player must manipulate them to create complete lines to score points.

## How to Play

- **Moving Blocks**: Use the 'A' key to move the current block left and the 'D' key to move it right.
- **Rotate Block**: Press the 'W' key to rotate the current block.
- **Drop Block Faster**: Press the 'S' key to drop the block faster.
- **Scoring**: Completing lines will remove them and award the player points.

## Features

- **Sound Management**: The game includes sound effects for gameplay interactions, managed using SFML Audio.
- **User Interface**: Implemented with ImGui, allowing the player to see the score and toggle sound effects.
- **Game Over**: If the player cannot fit a new block into the grid, the game ends, and the grid resets.

## Implementation Details

- **Grid Management**: The game grid tracks occupied cells to place and remove blocks.
- **Tetromino Shapes**: Different shapes are randomly generated using a predefined set of tetromino shapes.
- **Block Rotation**: Blocks can be rotated using the 'W' key if there is no collision with other blocks.
- **Audio Controls**: Sound effects can be enabled or disabled by toggling the checkbox in the user interface.
- **Threaded Sound Playback**: Sound playback is handled in separate threads to enable continuous monitoring of sound status.
- **Player Input**: GLFW input callbacks handle player key presses to interact with the game.

## Dependencies

- SFML: Required for audio management.
- ImGui: Used for creating the user interface.
- GLFW: Handles window management and user input.

## Running the Game

1. Clone the repository.
2. Build the project and link dependencies (SFML, ImGui, and GLFW).
3. Run the compiled executable to launch the game.

## Controls

- **'A'**: Move block left.
- **'D'**: Move block right.
- **'W'**: Rotate block.
- **'S'**: Drop block faster.



Feel free to explore and modify the code to enhance the Tetris game experience! Enjoy playing Tetris with ImGui and SFML Audio!
