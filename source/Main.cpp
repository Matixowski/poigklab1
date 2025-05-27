#include <vector>
#include <algorithm>
#include <functional> 
#include <memory>
#include <cstdlib>
#include <cmath>
#include <ctime>

#include <raylib.h>
#include <raymath.h>

bool isPaused = false;

float sgn(float x) {
	if (x < 0) {
		return 0.f;
	}
		return 1.f;
}

// --- UTILS ---
namespace Utils {
	inline static float RandomFloat(float min, float max) {
		return min + static_cast<float>(rand()) / RAND_MAX * (max - min);
	}
}

// --- TRANSFORM, PHYSICS, LIFETIME, RENDERABLE ---
struct TransformA {
	Vector2 position{};
	float rotation{};
};

struct Physics {
	Vector2 velocity{};
	float rotationSpeed{};
};

struct Renderable {
	enum Size { SMALL = 1, MEDIUM = 2, LARGE = 4 } size = SMALL;
};

// --- RENDERER ---
class Renderer {
public:
	static Renderer& Instance() {
		static Renderer inst;
		return inst;
	}

	void Init(int w, int h, const char* title) {
		InitWindow(w, h, title);
		SetTargetFPS(60);
		screenW = w;
		screenH = h;
	}

	void Begin() {
		BeginDrawing();
		ClearBackground(BLACK);
	}

	void End() {
		EndDrawing();
	}

	void DrawPoly(const Vector2& pos, int sides, float radius, float rot) {
		DrawPolyLines(pos, sides, radius, rot, WHITE);
	}

	int Width() const {
		return screenW;
	}

	int Height() const {
		return screenH;
	}

private:
	Renderer() = default;

	int screenW{};
	int screenH{};
};

// --- ASTEROID HIERARCHY ---
enum class WeaponType { LASER = 0, BULLET = 1, COUNT = 2 };
class Asteroid {
public:
	Asteroid(int screenW, int screenH) {
		init(screenW, screenH);
	}
	virtual ~Asteroid() = default;

	bool Update(float dt) {
		if (!isPaused) {
			transform.position = Vector2Add(transform.position, Vector2Scale(physics.velocity, dt));
			transform.rotation += physics.rotationSpeed * dt;
		}

		if (transform.position.x < -GetRadius() || transform.position.x > Renderer::Instance().Width() + GetRadius() ||
			transform.position.y < -GetRadius() || transform.position.y > Renderer::Instance().Height() + GetRadius())
			return false;
		return true;
	}
	virtual void Draw() const = 0;

	Vector2 GetPosition() const {
		return transform.position;
	}

	float constexpr GetRadius() const {
		return 16.f * (float)render.size;
	}

	int GetDamage() const {
		return baseDamage * static_cast<int>(render.size);
	}

	int GetSize() const {
		return static_cast<int>(render.size);
	}
	bool doesShoot() const {
		return shoot;
	}
	float GetBullets() const {
		return bullets;
	}
	void SetBullets(float blt) {
		bullets -= blt;
	}
	WeaponType getWeapon() const{
		return weapon;
	}

protected:
	void init(int screenW, int screenH) {
		// Choose size
		render.size = static_cast<Renderable::Size>(1 << GetRandomValue(0, 2));
		weapon = static_cast<WeaponType>(GetRandomValue(0, 1));
		// Spawn at random edge
		switch (GetRandomValue(0, 3)) {
		case 0:
			transform.position = { Utils::RandomFloat(0, screenW), -GetRadius() };
			break;
		case 1:
			transform.position = { screenW + GetRadius(), Utils::RandomFloat(0, screenH) };
			break;
		case 2:
			transform.position = { Utils::RandomFloat(0, screenW), screenH + GetRadius() };
			break;
		default:
			transform.position = { -GetRadius(), Utils::RandomFloat(0, screenH) };
			break;
		}

		// Aim towards center with jitter
		float maxOff = fminf(screenW, screenH) * 0.1f;
		float ang = Utils::RandomFloat(0, 2 * PI);
		float rad = Utils::RandomFloat(0, maxOff);
		Vector2 center = {
										 screenW * 0.5f + cosf(ang) * rad,
										 screenH * 0.5f + sinf(ang) * rad
		};

		Vector2 dir = Vector2Normalize(Vector2Subtract(center, transform.position));
		physics.velocity = Vector2Scale(dir, Utils::RandomFloat(SPEED_MIN, SPEED_MAX));
		physics.rotationSpeed = Utils::RandomFloat(ROT_MIN, ROT_MAX);

		transform.rotation = Utils::RandomFloat(0, 360);
	}

	TransformA transform;
	Physics    physics;
	Renderable render;

	int baseDamage = 0;
	bool shoot = false;
	float bullets = 20.0f;
	WeaponType weapon;
	static constexpr float LIFE = 10.f;
	static constexpr float SPEED_MIN = 125.f;
	static constexpr float SPEED_MAX = 250.f;
	static constexpr float ROT_MIN = 50.f;
	static constexpr float ROT_MAX = 240.f;
};

class TriangleAsteroid : public Asteroid {
public:
	TriangleAsteroid(int w, int h) : Asteroid(w, h) { baseDamage = 5; shoot = false; }
	void Draw() const override {
		Renderer::Instance().DrawPoly(transform.position, 3, GetRadius(), transform.rotation);
	}
};
class SquareAsteroid : public Asteroid {
public:
	SquareAsteroid(int w, int h) : Asteroid(w, h) { baseDamage = 10; shoot = false; }
	void Draw() const override {
		Renderer::Instance().DrawPoly(transform.position, 4, GetRadius(), transform.rotation);
	}
};
class PentagonAsteroid : public Asteroid {
public:
	PentagonAsteroid(int w, int h) : Asteroid(w, h) { baseDamage = 15; shoot = false; }
	void Draw() const override {
		Renderer::Instance().DrawPoly(transform.position, 5, GetRadius(), transform.rotation);
	}
};
class HexagonAsteroid : public Asteroid {
public:
	HexagonAsteroid(int w, int h) : Asteroid(w, h) { baseDamage = 20; shoot = true; }
	void Draw() const override {
		Renderer::Instance().DrawPoly(transform.position, 6, GetRadius(), transform.rotation);
	}
};

// Shape selector
enum class AsteroidShape { TRIANGLE = 3, SQUARE = 4, PENTAGON = 5, HEXAGON = 6, RANDOM = 0 };

// Factory
static inline std::unique_ptr<Asteroid> MakeAsteroid(int w, int h, AsteroidShape shape) {
	switch (shape) {
	case AsteroidShape::TRIANGLE:
		return std::make_unique<TriangleAsteroid>(w, h);
	case AsteroidShape::SQUARE:
		return std::make_unique<SquareAsteroid>(w, h);
	case AsteroidShape::PENTAGON:
		return std::make_unique<PentagonAsteroid>(w, h);
	case AsteroidShape::HEXAGON:
		return std::make_unique<HexagonAsteroid>(w, h);
	default: {
		return MakeAsteroid(w, h, static_cast<AsteroidShape>(3 + GetRandomValue(0, 2)));
	}
	}
}

// --- PROJECTILE HIERARCHY ---
//enum class WeaponType { LASER = 0, BULLET = 1, COUNT = 2 };
class Projectile {
public:
	Projectile(Vector2 pos, Vector2 vel, int dmg, WeaponType wt)
	{
		transform.position = pos;
		physics.velocity = vel;
		baseDamage = dmg;
		type = wt;
	}
	bool Update(float dt) {
		if (!isPaused) {
			transform.position = Vector2Add(transform.position, Vector2Scale(physics.velocity, dt));
		}

		if (transform.position.x < 0 ||
			transform.position.x > Renderer::Instance().Width() ||
			transform.position.y < 0 ||
			transform.position.y > Renderer::Instance().Height())
		{
			return true;
		}
		return false;
	}
	void Draw() const {
		if (type == WeaponType::BULLET) {
			DrawCircleV(transform.position, 5.f, WHITE);
		}
		else {
			static constexpr float LASER_LENGTH = 30.f;
			Rectangle lr = { 0.f, 0.f, 1.f, 1.f };
			float xs = sgn(physics.velocity.x);
			float ys = sgn(physics.velocity.y);
			if (physics.velocity.x != 0) {
				lr = { transform.position.x - xs * LASER_LENGTH, transform.position.y - ys * 2.f, LASER_LENGTH, 4.f };
			}
			else {
				lr = { transform.position.x - xs * 2.f, transform.position.y - ys * LASER_LENGTH, 4.f, LASER_LENGTH };
			}
			DrawRectangleRec(lr, RED);
		}
	}
	Vector2 GetPosition() const {
		return transform.position;
	}

	float GetRadius() const {
		return (type == WeaponType::BULLET) ? 5.f : 2.f;
	}

	int GetDamage() const {
		return baseDamage;
	}

private:
	TransformA transform;
	Physics    physics;
	int        baseDamage;
	WeaponType type;
};

inline static Projectile MakeProjectile(WeaponType wt,
	const Vector2 pos,
	float speedx,
	float speedy)
{
	Vector2 vel{ speedx, -speedy };
	if (wt == WeaponType::LASER) {
		return Projectile(pos, vel, 20, wt);
	}
	else {
		return Projectile(pos, vel, 10, wt);
	}
}

// --- SHIP HIERARCHY ---
class Ship {
public:
	Ship(int screenW, int screenH) {
		transform.position = {
												 screenW * 0.5f,
												 screenH * 0.5f
		};
		hp = 100;
		speed = 250.f;
		alive = true;

		// per-weapon fire rate & spacing
		fireRateLaser = 18.f; // shots/sec
		fireRateBullet = 22.f;
		spacingLaser = 40.f; // px between lasers
		spacingBullet = 20.f;
	}
	virtual ~Ship() = default;
	virtual void Update(float dt) = 0;
	virtual void Draw() const = 0;

	void TakeDamage(int dmg) {
		if (!alive) return;
		hp -= dmg;
		if (hp <= 0) alive = false;
	}

	bool IsAlive() const {
		return alive;
	}

	Vector2 GetPosition() const {
		return transform.position;
	}

	virtual float GetRadius() const = 0;

	int GetHP() const {
		return hp;
	}

	float GetFireRate(WeaponType wt) const {
		return (wt == WeaponType::LASER) ? fireRateLaser : fireRateBullet;
	}

	float GetSpacing(WeaponType wt) const {
		return (wt == WeaponType::LASER) ? spacingLaser : spacingBullet;
	}

protected:
	TransformA transform;
	int        hp;
	float      speed;
	bool       alive;
	float      fireRateLaser;
	float      fireRateBullet;
	float      spacingLaser;
	float      spacingBullet;
};

class Consumable {
public:
	Consumable(int val, Vector2 pos) {
		value = GetRandomValue(val - 5, val);
		transform.position = pos;
	}
	int getValue() {
		return value;
	}
	float getLifeTime() {
		return lifetime;
	}
	void addLifeTime(float time) {
		lifetime += time;
	}
	void Draw() const{
		Rectangle cr = { transform.position.x - 5, transform.position.y - 5, 10.f, 10.f };
		DrawRectangleRec(cr, PINK);
	}
	Vector2 getPosition() {
		return transform.position;
	}
private:
	int value;
	float lifetime = 0.f;
	TransformA transform;
};

class PlayerShip :public Ship {
public:
	PlayerShip(int w, int h) : Ship(w, h) {
		texture = LoadTexture("spaceship1.png");
		GenTextureMipmaps(&texture);                                                        // Generate GPU mipmaps for a texture
		SetTextureFilter(texture, 2);
		scale = 0.25f;
		score = 0;
	}
	~PlayerShip() {
		UnloadTexture(texture);
	}

	void Update(float dt) override {
		if (alive) {
			if (IsKeyDown(KEY_W)) transform.position.y -= speed * dt;
			if (IsKeyDown(KEY_S)) transform.position.y += speed * dt;
			if (IsKeyDown(KEY_A)) transform.position.x -= speed * dt;
			if (IsKeyDown(KEY_D)) transform.position.x += speed * dt;
		}
		else {
			transform.position.y += speed * dt;
		}
	}

	void Draw() const override {
		if (!alive && fmodf(GetTime(), 0.4f) > 0.2f) return;
		Vector2 dstPos = {
										 transform.position.x - (texture.width * scale) * 0.5f,
										 transform.position.y - (texture.height * scale) * 0.5f
		};
		DrawTextureEx(texture, dstPos, 0.0f, scale, WHITE);
	}

	float GetRadius() const override {
		return (texture.width * scale) * 0.5f;
	}
	int getScore() {
		return score;
	}
	void addScore(int a) {
		score += a;
	}

private:
	Texture2D texture;
	float     scale;
	int score;
};

// --- APPLICATION ---
class Application {
public:
	static Application& Instance() {
		static Application inst;
		return inst;
	}

	void Run() {
		srand(static_cast<unsigned>(time(nullptr)));
		Renderer::Instance().Init(C_WIDTH, C_HEIGHT, "Asteroids OOP");

		auto player = std::make_unique<PlayerShip>(C_WIDTH, C_HEIGHT);

		float spawnTimer = 0.f;
		float spawnInterval = Utils::RandomFloat(C_SPAWN_MIN, C_SPAWN_MAX);
		WeaponType currentWeapon = WeaponType::LASER;
		float shotTimer = 0.f;

		while (!WindowShouldClose()) {
			float dt = GetFrameTime();
			spawnTimer += dt;

			if (IsKeyPressed(KEY_P) && player->IsAlive()) {
				if (isPaused) {
					isPaused = false;
				}
				else {
					isPaused = true;
				}
			}

			// Update player
			if (!isPaused) {
				player->Update(dt);
			}

			// Restart logic
			if (!player->IsAlive() && IsKeyPressed(KEY_R)) {
				player = std::make_unique<PlayerShip>(C_WIDTH, C_HEIGHT);
				asteroids.clear();
				projectiles.clear();
				aprojectiles.clear();
				consumables.clear();
				spawnTimer = 0.f;
				spawnInterval = Utils::RandomFloat(C_SPAWN_MIN, C_SPAWN_MAX);
			}
			// Asteroid shape switch
			if (IsKeyPressed(KEY_ONE)) {
				currentShape = AsteroidShape::TRIANGLE;
			}
			if (IsKeyPressed(KEY_TWO)) {
				currentShape = AsteroidShape::SQUARE;
			}
			if (IsKeyPressed(KEY_THREE)) {
				currentShape = AsteroidShape::PENTAGON;
			}
			if (IsKeyPressed(KEY_FOUR)) {
				currentShape = AsteroidShape::HEXAGON;
			}
			if (IsKeyPressed(KEY_FIVE)) {
				currentShape = AsteroidShape::RANDOM;
			}


			// Weapon switch
			if (IsKeyPressed(KEY_TAB)) {
				currentWeapon = static_cast<WeaponType>((static_cast<int>(currentWeapon) + 1) % static_cast<int>(WeaponType::COUNT));
			}

			// Shooting
			{
				if (player->IsAlive() && IsKeyDown(KEY_SPACE) && !isPaused) {
					shotTimer += dt;
					float interval = 1.f / player->GetFireRate(currentWeapon);
					float projSpeed = player->GetSpacing(currentWeapon) * player->GetFireRate(currentWeapon);

					while (shotTimer >= interval) {
						Vector2 p = player->GetPosition();
						p.y -= player->GetRadius();
						projectiles.push_back(MakeProjectile(currentWeapon, p, 0.f, projSpeed));
						shotTimer -= interval;
					}
				}
				else {
					float maxInterval = 1.f / player->GetFireRate(currentWeapon);

					if (shotTimer > maxInterval) {
						shotTimer = fmodf(shotTimer, maxInterval);
					}
				}
			}

			// Spawn asteroids
			if (spawnTimer >= spawnInterval && asteroids.size() < MAX_AST && !isPaused) {
				asteroids.push_back(MakeAsteroid(C_WIDTH, C_HEIGHT, currentShape));
				spawnTimer = 0.f;
				spawnInterval = Utils::RandomFloat(C_SPAWN_MIN, C_SPAWN_MAX);
			}

			for (auto pait = asteroids.begin(); pait != asteroids.end(); ++pait) {
				if ((*pait)->doesShoot() && !isPaused) {
					float bullets = (*pait)->GetBullets();
					if (bullets>0 && bullets - floorf(bullets) > 1-0.5*dt) {
						Vector2 ap = (*pait)->GetPosition();
						float r = (*pait)->GetRadius();
						ap.y -= r;
						WeaponType wptp = (*pait)->getWeapon();
						float prspd = 0.f;
						(wptp == WeaponType::LASER) ? prspd = 720.f : prspd = 440.f;
						aprojectiles.push_back(MakeProjectile(wptp, ap, 0.f, prspd));
						ap.y += 2 * r;
						aprojectiles.push_back(MakeProjectile(wptp, ap, 0.f, -prspd));
						ap.y -= r;
						ap.x -= r;
						aprojectiles.push_back(MakeProjectile(wptp, ap, -prspd, 0.f));
						ap.x += 2 * r;
						aprojectiles.push_back(MakeProjectile(wptp, ap, prspd, 0.f));
					}
					(*pait)->SetBullets(0.5*dt);

				}
			}

			// Update projectiles - check if in boundries and move them forward
			{
				auto projectile_to_remove = std::remove_if(projectiles.begin(), projectiles.end(),
					[dt](auto& projectile) {
						return projectile.Update(dt);
					});
				projectiles.erase(projectile_to_remove, projectiles.end());
			}

			{
				auto aprojectile_to_remove = std::remove_if(aprojectiles.begin(), aprojectiles.end(),
					[dt](auto& aprojectile) {
						return aprojectile.Update(dt);
					});
				aprojectiles.erase(aprojectile_to_remove, aprojectiles.end());
			}

			// Projectile-Asteroid collisions O(n^2)
			for (auto pit = projectiles.begin(); pit != projectiles.end();) {
				bool removed = false;

				for (auto ait = asteroids.begin(); ait != asteroids.end(); ++ait) {
					float dist = Vector2Distance((*pit).GetPosition(), (*ait)->GetPosition());
					if (dist < (*pit).GetRadius() + (*ait)->GetRadius()) {
						ait = asteroids.erase(ait);
						pit = projectiles.erase(pit);
						int rnd = GetRandomValue(0, 100);
						if (rnd > 70) {
							consumables.push_back(Consumable((*ait)->GetDamage(), (*ait)->GetPosition()));
						}
						player->addScore((*ait)->GetDamage());
						removed = true;
						break;
					}
				}
				if (!removed) {
					++pit;
				}
			}

			// Projectile-Player collision
			for (auto apit = aprojectiles.begin(); apit != aprojectiles.end();) {
				bool aremoved = false;

				float adist = Vector2Distance((*apit).GetPosition(), player->GetPosition());
				if (adist < (*apit).GetRadius() + player->GetRadius()) {
					apit = aprojectiles.erase(apit);
					player->TakeDamage((*apit).GetDamage());
					aremoved = true;
					break;
				}
				if (!aremoved) {
					++apit;
				}
			}

			// Consumable-Player collision
			for (auto cpit = consumables.begin(); cpit != consumables.end();) {
				bool cremoved = false;
				if (!isPaused) {
					(*cpit).addLifeTime(dt);
				}
				float cdist = Vector2Distance((*cpit).getPosition(), player->GetPosition());
				if (cdist < 5 + player->GetRadius()) {
					cpit = consumables.erase(cpit);
					player->TakeDamage(-(*cpit).getValue());
					cremoved = true;
					break;
				}
				if ((*cpit).getLifeTime() > 5) {
					cpit = consumables.erase(cpit);
					cremoved = true;
					break;
				}
				if (!cremoved) {
					++cpit;
				}
			}

			// Asteroid-Ship collisions
			{
				auto remove_collision =
					[&player, dt](auto& asteroid_ptr_like) -> bool {
					if (player->IsAlive()) {
						float dist = Vector2Distance(player->GetPosition(), asteroid_ptr_like->GetPosition());

						if (dist < player->GetRadius() + asteroid_ptr_like->GetRadius()) {
							player->TakeDamage(asteroid_ptr_like->GetDamage());
							return true; // Mark asteroid for removal due to collision
						}
					}
					if (!asteroid_ptr_like->Update(dt)) {
						return true;
					}
					return false; // Keep the asteroid
					};
				auto asteroid_to_remove = std::remove_if(asteroids.begin(), asteroids.end(), remove_collision);
				asteroids.erase(asteroid_to_remove, asteroids.end());
			}

			// Render everything
			{
				Renderer::Instance().Begin();

				for (const auto& projPtr : projectiles) {
					projPtr.Draw();
				}
				for (const auto& aprojPtr : aprojectiles) {
					aprojPtr.Draw();
				}
				for (const auto& astPtr : asteroids) {
					astPtr->Draw();
				}
				for (const auto& consPtr : consumables) {
					consPtr.Draw();
				}

				player->Draw();

				const char* fpsc = TextFormat("FPS: %d", GetFPS());
				int fpscs = MeasureText(fpsc, 20);
				DrawText(fpsc, C_WIDTH - fpscs - 10, 10, 20, RED);

				if (player->IsAlive()) {
					DrawText(TextFormat("HP: %d", player->GetHP()),
						10, 10, 20, GREEN);
					DrawText(TextFormat("Score: %06i", player->getScore()), 10, 70, 20, RED);

					const char* weaponName = (currentWeapon == WeaponType::LASER) ? "LASER" : "BULLET";
					DrawText(TextFormat("Weapon: %s", weaponName),
						10, 40, 20, BLUE);
				}
				else {
					const char* gmov = "GAME OVER";
					Vector2 gmovs = MeasureTextEx(GetFontDefault(), gmov, 40, 0);
					const char* gmovscore = TextFormat("Score: %06i", player->getScore());
					Vector2 gmovscores = MeasureTextEx(GetFontDefault(), gmovscore, 20, 0);
					const char* gmovr = "Press R to restart";
					Vector2 gmovrs = MeasureTextEx(GetFontDefault(), gmovr, 20, 0);
					DrawText(gmov, (C_WIDTH - gmovs.x) / 2, (C_HEIGHT - gmovs.y) / 2, 40, WHITE);
					DrawText(gmovscore, (C_WIDTH - gmovscores.x) / 2, (C_HEIGHT - gmovscores.y) / 2 + 50, 20, WHITE);
					DrawText(gmovr, (C_WIDTH - gmovrs.x) / 2, (C_HEIGHT - gmovrs.y) / 2 + 100, 20, RED);

				}

				if (isPaused) {
					const char* gmp = "PAUSED";
					int gmps = MeasureText(gmp, 60);
					DrawText(gmp, (C_WIDTH - gmps) / 2, 50, 60, PURPLE);
					const char* h0 = "Controls";
					const char* h1 = "1 - Change asteroid shape to triangle";
					const char* h2 = "2 - Change asteroid shape to square";
					const char* h3 = "3 - Change asteroid shape to pentagon";
					const char* h4 = "4 - Change asteroid shape to hexagon";
					const char* h5 = "5 - Change asteroid shape to random";
					const char* h6 = "TAB - change weapon";
					const char* h7 = "ESC - exit";
					DrawText(h0, (C_WIDTH - MeasureText(h0, 40)) / 2, 120, 40, PURPLE);
					DrawText(h1, (C_WIDTH - MeasureText(h1, 20)) / 2, 180, 20, PURPLE);
					DrawText(h2, (C_WIDTH - MeasureText(h2, 20)) / 2, 210, 20, PURPLE);
					DrawText(h3, (C_WIDTH - MeasureText(h3, 20)) / 2, 240, 20, PURPLE);
					DrawText(h4, (C_WIDTH - MeasureText(h4, 20)) / 2, 270, 20, PURPLE);
					DrawText(h5, (C_WIDTH - MeasureText(h5, 20)) / 2, 300, 20, PURPLE);
					DrawText(h6, (C_WIDTH - MeasureText(h6, 20)) / 2, 330, 20, PURPLE);
					DrawText(h7, (C_WIDTH - MeasureText(h7, 20)) / 2, 360, 20, PURPLE);
				}

				Renderer::Instance().End();
			}
		}
	}

private:
	Application()
	{
		asteroids.reserve(1000);
		projectiles.reserve(10'000);
		aprojectiles.reserve(10'000);
	};

	std::vector<std::unique_ptr<Asteroid>> asteroids;
	std::vector<Projectile> projectiles;
	std::vector<Projectile> aprojectiles;
	std::vector<Consumable> consumables;

	AsteroidShape currentShape = AsteroidShape::TRIANGLE;

	static constexpr int C_WIDTH = 1000;
	static constexpr int C_HEIGHT = 1000;
	static constexpr size_t MAX_AST = 150;
	static constexpr float C_SPAWN_MIN = 0.5f;
	static constexpr float C_SPAWN_MAX = 3.0f;

	static constexpr int C_MAX_ASTEROIDS = 1000;
	static constexpr int C_MAX_PROJECTILES = 10'000;
	static constexpr int C_MAX_APROJECTILES = 10'000;
	static constexpr int C_MAX_CONSUMABLES = 100;
};

int main() {
	Application::Instance().Run();
	return 0;
}
