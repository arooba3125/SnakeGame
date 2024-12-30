// Include necessary headers
#include <iostream>      // For console input and output
#include <raylib.h>      // For graphical rendering and game development utilities
#include <raymath.h>     // For vector and matrix operations
#include <deque>         // For using the deque container from the standard library
using namespace std;     // Standard namespace to avoid prefixing std::

// Define colors used in the game
Color light = { 173,204,96,255 };  // Light color for background
Color dark = { 43,51,24,255 };     // Dark color for drawing texts and borders

// Game settings
int cellSize = 30;                // Size of each cell in the game grid
int cellCount = 25;               // Number of cells in one row or column
int offset = 75;                  // Offset from window edges to the game grid
double gameUpdateTime = 0;        // Last game update time
bool gameOver = false;            // Flag to check if the game is over
bool allowMove = false;           // Flag to allow snake movement
string winnerMessage = "";        // Message to display the winner

// Function to trigger events based on time interval
bool eventTriggered(double interval, double& lastUpdateTime) {
	double currentTime = GetTime();     // Get current time
	if (currentTime - lastUpdateTime >= interval) {
		lastUpdateTime = currentTime;  // Update last update time
		return true;                    // Event triggered
	}
	return false;                       // Event not triggered
}

// Function to check if a vector element exists in a deque
bool elementInDeque(Vector2 element, deque<Vector2> deque) {
	for (unsigned int i = 0; i < deque.size(); i++) {
		if (Vector2Equals(deque[i], element)) {
			return true;    // Element found
		}
	}
	return false;           // Element not found
}

// Food class for managing food items in the game
class Food {
public:
	Vector2 pos;          // Position of the food
	Texture2D texture;    // Texture of the food

	// Constructor for normal food
	Food(deque<Vector2> snake1_body, deque<Vector2> snake2_body) {
		Image image = LoadImage("Graphics/food.png"); // Load food image
		texture = LoadTextureFromImage(image);       // Create texture from image
		UnloadImage(image);                         // Unload the image to free memory
		pos = GenRandPos(snake1_body, snake2_body); // Generate random position for food
	}

	// Constructor for power-up food
	Food(deque<Vector2> snake_body, deque<Vector2> snake_body1, int powerup) {
		Image image = LoadImage("Graphics/powerup.png"); // Load power-up image
		texture = LoadTextureFromImage(image);          // Create texture from image
		UnloadImage(image);                            // Unload the image to free memory
		pos = GenRandPos(snake_body, snake_body1);     // Generate random position for power-up
	}

	// Destructor to unload texture
	~Food() {
		UnloadTexture(texture);                       // Unload texture to free memory
	}

	// Function to draw the food on the game screen
	void draw() {
		DrawTexture(texture, offset + pos.x * cellSize, offset + pos.y * cellSize, WHITE);
	}

	// Function to generate a random cell position
	Vector2 randomCell() {
		float x = GetRandomValue(0, cellCount - 1);
		float y = GetRandomValue(0, cellCount - 1);
		return Vector2{ x, y };
	}

	// Function to generate a random position not occupied by snakes
	Vector2 GenRandPos(deque<Vector2> snake1_body, deque<Vector2> snake2_body) {
		Vector2 position = randomCell();
		while (elementInDeque(position, snake1_body) || elementInDeque(position, snake2_body)) {
			position = randomCell();  // Keep generating until a free spot is found
		}
		return position;
	}
};

// Snake class for managing snake objects in the game
class Snake {
public:
	deque<Vector2> body;  // Deque to store body segments of the snake
	Vector2 direction;    // Current moving direction of the snake
	Color color;          // Color of the snake
	bool addSegment = false;  // Flag to determine whether to add a new segment

	// Constructor to initialize the snake
	Snake(Vector2 startPos, Vector2 startDirection, Color snakeColor) {
		direction = startDirection;  // Set initial direction
		color = snakeColor;          // Set snake color
		body.push_back(startPos);    // Add head to the body
		body.push_back(Vector2Subtract(startPos, startDirection));  // Add second segment
		body.push_back(Vector2Subtract(body.back(), startDirection));  // Add third segment
	}

	// Function to draw the snake on the game screen
	void Draw() {
		for (const auto& segment : body) {
			float x = segment.x;
			float y = segment.y;
			Rectangle rect = { offset + x * cellSize, offset + y * cellSize, (float)cellSize, (float)cellSize };
			DrawRectangleRounded(rect, 0.5, 6, color);  // Draw each segment as a rounded rectangle
		}
	}

	// Function to update the snake's position
	void update() {
		body.push_front(Vector2Add(body[0], direction));  // Move the head in the current direction
		if (!addSegment) {
			body.pop_back();  // Remove the tail segment if not growing
		} else {
			addSegment = false;  // Reset the flag after adding a segment
		}
	}

	// Function to reset the snake to the initial state
	void reset(Vector2 startPos, Vector2 startDirection) {
		body.clear();  // Clear the existing body
		direction = startDirection;  // Reset direction
		body.push_back(startPos);  // Add head
		body.push_back(Vector2Subtract(startPos, startDirection));  // Add second segment
		body.push_back(Vector2Subtract(body.back(), startDirection));  // Add third segment
	}
};

// Game class for managing the overall game logic
class Game {
public:
	Snake snake1;  // First snake object
	Snake snake2;  // Second snake object
	Food food;     // Food object
	Food powerup;  // Power-up object
	bool running = true;  // Flag to check if the game is running
	int score1 = 0;  // Score for first snake
	int score2 = 0;  // Score for second snake
	bool showPowerup = false;  // Flag to display the power-up
	double powerupOnTime = 0;  // Time when the power-up appeared
	double powerupOffTime = 0;  // Time when the power-up disappeared
	double powerupTimeGap = GetRandomValue(15, 16);  // Random time gap for the power-up to appear

	// Sounds for various game events
	Sound eatSound;     // Sound when the snake eats food
	Sound hitSound;     // Sound when the snake hits a wall or itself
	Sound powerupSound;  // Sound when the snake eats a power-up

	// Constructor to initialize the game
	Game()
		: snake1(Vector2{ 6, 9 }, Vector2{ 1, 0 }, DARKGREEN),
		snake2(Vector2{ 18, 9 }, Vector2{ -1, 0 }, DARKBLUE),
		food(snake1.body, snake2.body),
		powerup(snake1.body, snake2.body, 1) {
		InitAudioDevice();  // Initialize audio device
		eatSound = LoadSound("Sounds/eat.mp3");  // Load eating sound
		hitSound = LoadSound("Sounds/wall.mp3");  // Load hitting sound
		powerupSound = LoadSound("Sounds/powerup.mp3");  // Load power-up sound
	}

	// Destructor to unload sounds and close audio device
	~Game() {
		UnloadSound(eatSound);  // Unload eating sound
		UnloadSound(hitSound);  // Unload hitting sound
		UnloadSound(powerupSound);  // Unload power-up sound
		CloseAudioDevice();  // Close the audio device
	}

	// Function to draw game elements
	void draw() {
		snake1.Draw();  // Draw first snake
		snake2.Draw();  // Draw second snake
		food.draw();  // Draw food
		if (showPowerup) {
			powerup.draw();  // Draw power-up if it is visible
		}
	}

	// Function to update game state
	void update() {
		if (running) {
			snake1.update();  // Update first snake
			snake2.update();  // Update second snake
			checkFoodCollision(snake1, score1);  // Check collision with food for first snake
			checkFoodCollision(snake2, score2);  // Check collision with food for second snake
			checkPowerupCollision(snake1, score1);  // Check collision with power-up for first snake
			checkPowerupCollision(snake2, score2);  // Check collision with power-up for second snake
			checkCollisions();  // Check for any other collisions
			togglePowerupOff();  // Manage power-up visibility
			togglePowerupOn();  // Manage power-up appearance
		}
	}

	// Function to check food collision for a snake
	void checkFoodCollision(Snake& snake, int& score) {
		if (Vector2Equals(snake.body[0], food.pos)) {
			food.pos = food.GenRandPos(snake1.body, snake2.body);  // Generate new food position
			snake.addSegment = true;  // Flag to add a new segment to the snake
			PlaySound(eatSound);  // Play eating sound
			score++;  // Increase score
		}
	}

	// Function to check power-up collision for a snake
	void checkPowerupCollision(Snake& snake, int& score) {
		if (Vector2Equals(snake.body[0], powerup.pos)) {
			powerup.pos = { -1, -1 };  // Invalidate power-up position
			showPowerup = false;  // Hide the power-up
			snake.addSegment = true;  // Flag to add a new segment to the snake
			PlaySound(powerupSound);  // Play power-up sound
			score += 5;  // Increase score by 5
		}
	}

	// Function to check for collisions involving snakes
	void checkCollisions() {
		if (isOutOfBounds(snake1)) declareWinner(2);  // Declare second snake as winner if first snake is out of bounds
		if (isOutOfBounds(snake2)) declareWinner(1);  // Declare first snake as winner if second snake is out of bounds
		if (selfCollision(snake1)) declareWinner(2);  // Declare second snake as winner if first snake collides with itself
		if (selfCollision(snake2)) declareWinner(1);  // Declare first snake as winner if second snake collides with itself
		if (elementInDeque(snake1.body[0], snake2.body)) declareWinner(2);  // Declare second snake as winner if first snake collides with second snake
		if (elementInDeque(snake2.body[0], snake1.body)) declareWinner(1);  // Declare first snake as winner if second snake collides with first snake
	}

	// Function to check if a snake is out of bounds
	bool isOutOfBounds(Snake& snake) {
		return snake.body[0].x < 0 || snake.body[0].x >= cellCount ||
			snake.body[0].y < 0 || snake.body[0].y >= cellCount;  // Return true if the head is outside the game grid
	}

	// Function to check if a snake has collided with itself
	bool selfCollision(Snake& snake) {
		deque<Vector2> headlessBody = snake.body;  // Copy the snake body
		headlessBody.pop_front();  // Remove the head
		return elementInDeque(snake.body[0], headlessBody);  // Check if the head collides with any other body segment
	}

	// Function to manage power-up disappearance
	void togglePowerupOff() {
		if (showPowerup && eventTriggered(10, powerupOnTime)) {
			showPowerup = false;  // Hide the power-up
			powerup.pos = { -1, -1 };  // Invalidate power-up position
		}
	}

	// Function to manage power-up appearance
	void togglePowerupOn() {
		if (!showPowerup && eventTriggered(powerupTimeGap, powerupOffTime)) {
			showPowerup = true;  // Show the power-up
			powerupOnTime = GetTime();  // Set the appearance time
			powerup.pos = powerup.GenRandPos(snake1.body, snake2.body);  // Generate new power-up position
		}
	}

	// Function to declare the winner and end the game
	void declareWinner(int winner) {
		running = false;  // Stop the game
		gameOver = true;  // Set game over flag
		winnerMessage = (winner == 1) ? "Player 1 Wins!" : "Player 2 Wins!";  // Set winner message
	}

	// Function to reset the game to the initial state
	void reset() {
		snake1.reset(Vector2{ 6, 9 }, Vector2{ 1, 0 });  // Reset first snake
		snake2.reset(Vector2{ 18, 9 }, Vector2{ -1, 0 });  // Reset second snake
		food.pos = food.GenRandPos(snake1.body, snake2.body);  // Generate new food position
		powerup.pos = { -1, -1 };  // Invalidate power-up position
		showPowerup = false;  // Hide the power-up
		score1 = 0;  // Reset first snake's score
		score2 = 0;  // Reset second snake's score
		running = true;  // Start the game
		gameOver = false;  // Clear game over flag
		winnerMessage = "";  // Clear winner message
	}
};

// Main function to initialize and run the game
int main() {
	InitWindow(2 * offset + cellSize * cellCount, 2 * offset + cellSize * cellCount, "2-Player Snake with Powerups");  // Initialize the game window
	SetTargetFPS(60);  // Set the target frames per second

	Game game;  // Create a Game object

	// Game loop
	while (!WindowShouldClose()) {
		BeginDrawing();  // Start drawing
		ClearBackground(light);  // Clear the background with light color

		if (!gameOver && eventTriggered(0.2, gameUpdateTime)) {
			allowMove = true;  // Allow snake movement
			game.update();  // Update the game
		}
		if (gameOver && IsKeyPressed(KEY_SPACE)) {
			game.reset();  // Reset the game if space key is pressed
		}
		if (gameOver) {
			DrawText(winnerMessage.c_str(), offset + 100, offset + (cellSize * cellCount) / 2, 40, RED);  // Draw winner message
			DrawText("Press SPACE to Restart", offset + 100, offset + (cellSize * cellCount) / 2 + 50, 20, RED);  // Draw restart message
		} else {
			game.draw();  // Draw the game elements
		}
		if (!gameOver && allowMove) {
			// Handle input for first snake
			if (IsKeyPressed(KEY_UP) && game.snake1.direction.y != 1) {
				game.snake1.direction = { 0, -1 };  // Change direction to up
				allowMove = false;  // Disallow further movement until next update
			}
			if (IsKeyPressed(KEY_DOWN) && game.snake1.direction.y != -1) {
				game.snake1.direction = { 0, 1 };  // Change direction to down
				allowMove = false;  // Disallow further movement until next update
			}
			if (IsKeyPressed(KEY_RIGHT) && game.snake1.direction.x != -1) {
				game.snake1.direction = { 1, 0 };  // Change direction to right
				allowMove = false;  // Disallow further movement until next update
			}
			if (IsKeyPressed(KEY_LEFT) && game.snake1.direction.x != 1) {
				game.snake1.direction = { -1, 0 };  // Change direction to left
				allowMove = false;  // Disallow further movement until next update
			}

			// Handle input for second snake
			if (IsKeyPressed(KEY_W) && game.snake2.direction.y != 1) {
				game.snake2.direction = { 0, -1 };  // Change direction to up
				allowMove = false;  // Disallow further movement until next update
			}
			if (IsKeyPressed(KEY_S) && game.snake2.direction.y != -1) {
				game.snake2.direction = { 0, 1 };  // Change direction to down
				allowMove = false;  // Disallow further movement until next update
			}
			if (IsKeyPressed(KEY_D) and game.snake2.direction.x != -1) {
				game.snake2.direction = { 1, 0 };  // Change direction to right
				allowMove = false;  // Disallow further movement until next update
			}
			if (IsKeyPressed(KEY_A) and game.snake2.direction.x != 1) {
				game.snake2.direction = { -1, 0 };  // Change direction to left
				allowMove = false;  // Disallow further movement until next update
			}
		}

		// Draw the outer border for the game grid
		DrawRectangleLinesEx(Rectangle{ (float)offset - 5, (float)offset - 5, (float)cellSize * cellCount + 10, (float)cellSize * cellCount + 10 }, 5, dark);
		// Draw the game title
		DrawText("2-Player Snake ", offset - 5, 20, 40, dark);
		// Draw the scores for both players
		DrawText(TextFormat("P1 Score: %02i", game.score1), offset - 5, offset + cellSize * cellCount + 10, 20, dark);
		DrawText(TextFormat("P2 Score: %02i", game.score2), offset + 300, offset + cellSize * cellCount + 10, 20, dark);

		EndDrawing();  // End drawing
	}

	CloseWindow();  // Close the game window
}
