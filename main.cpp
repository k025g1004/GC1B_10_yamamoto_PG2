#include <Novice.h>
#include <cstring>
#include <vector>
#include <memory>
#include <cstdlib>

//--------------------------------
// 定数
//--------------------------------
const char kWindowTitle[] = "GC1B_10_ヤマモト_ケンショウ_SIMPLE_STG";

const int kWindowWidth = 1280;
const int kWindowHeight = 720;

//--------------------------------
// 構造体
//--------------------------------
struct Vector2 {
	float x;
	float y;
};

//--------------------------------
// 当たり判定
//--------------------------------
static bool IsHit(Vector2 a, float ar, Vector2 b, float br) {
	float dx = a.x - b.x;
	float dy = a.y - b.y;
	float dist = dx * dx + dy * dy;
	float r = ar + br;
	return dist <= r * r;
}

//--------------------------------
// Bulletクラス
//--------------------------------
class Bullet {
public:
	Bullet() : isActive_(false), speed_(10.0f), radius_(5.0f) {}

	void Fire(Vector2 pos) {
		pos_ = pos;
		isActive_ = true;
	}

	void Update() {
		if (!isActive_) return;
		pos_.x += speed_;
		if (pos_.x > kWindowWidth) {
			isActive_ = false;
		}
	}

	void Draw() const {
		if (!isActive_) return;
		Novice::DrawEllipse(
			(int)pos_.x, (int)pos_.y,
			(int)radius_, (int)radius_,
			0.0f, RED, kFillModeSolid
		);
	}

	bool IsActive() const { return isActive_; }
	void Deactivate() { isActive_ = false; }

	Vector2 GetPos() const { return pos_; }
	float GetRadius() const { return radius_; }

private:
	Vector2 pos_;
	float speed_;
	float radius_;
	bool isActive_;
};

//--------------------------------
// Playerクラス
//--------------------------------
class Player {
public:
	Player() {
		pos_ = { 200.0f, kWindowHeight / 2.0f };
		speed_ = 5.0f;
		radius_ = 20.0f;
		hp_ = 500;
	}

	void Update(char* keys, char* preKeys) {
		if (keys[DIK_W]) pos_.y -= speed_;
		if (keys[DIK_S]) pos_.y += speed_;
		if (keys[DIK_A]) pos_.x -= speed_;
		if (keys[DIK_D]) pos_.x += speed_;

		if (pos_.x < radius_) pos_.x = radius_;
		if (pos_.x > kWindowWidth - radius_) pos_.x = kWindowWidth - radius_;
		if (pos_.y < radius_) pos_.y = radius_;
		if (pos_.y > kWindowHeight - radius_) pos_.y = kWindowHeight - radius_;

		if (preKeys[DIK_SPACE] == 0 && keys[DIK_SPACE] != 0) {
			Shoot();
		}

		for (auto& bullet : bullets_) {
			bullet.Update();
		}
	}

	void Draw() const {
		Novice::DrawEllipse(
			(int)pos_.x, (int)pos_.y,
			(int)radius_, (int)radius_,
			0.0f, WHITE, kFillModeSolid
		);

		for (const auto& bullet : bullets_) {
			bullet.Draw();
		}
	}

	void Damage(int dmg) { hp_ -= dmg; }
	int GetHP() const { return hp_; }

	Vector2 GetPos() const { return pos_; }
	float GetRadius() const { return radius_; }
	std::vector<Bullet>& GetBullets() { return bullets_; }

private:
	void Shoot() {
		for (auto& bullet : bullets_) {
			if (!bullet.IsActive()) {
				bullet.Fire(pos_);
				return;
			}
		}
		bullets_.emplace_back();
		bullets_.back().Fire(pos_);
	}

	Vector2 pos_;
	float speed_;
	float radius_;
	int hp_;
	std::vector<Bullet> bullets_;
};

//--------------------------------
// Enemyクラス
//--------------------------------
class Enemy {
public:
	Enemy() { Respawn(); }

	void Update() {
		pos_.x -= speed_;
		if (pos_.x < -radius_) {
			Respawn();
		}
	}

	void Draw() const {
		Novice::DrawEllipse(
			(int)pos_.x, (int)pos_.y,
			(int)radius_, (int)radius_,
			0.0f, BLUE, kFillModeSolid
		);
	}

	void Respawn() {
		pos_.x = (float)(kWindowWidth + rand() % 300);
		pos_.y = (float)(100 + rand() % (kWindowHeight - 200));
		speed_ = 3.0f + rand() % 3;
		radius_ = 20.0f;
	}

	Vector2 GetPos() const { return pos_; }
	float GetRadius() const { return radius_; }

private:
	Vector2 pos_;
	float speed_;
	float radius_;
};

//--------------------------------
// Scene 基底クラス
//--------------------------------
class SceneBase {
public:
	virtual ~SceneBase() {}
	virtual void Update(char*, char*) = 0;
	virtual void Draw() = 0;
	virtual bool IsEnd() const = 0;
	virtual std::unique_ptr<SceneBase> NextScene() = 0;
};

//--------------------------------
// 前方宣言
//--------------------------------
class TitleScene;
class GameScene;
class ClearScene;
class GameOverScene;

//--------------------------------
// TitleScene
//--------------------------------
class TitleScene : public SceneBase {
public:
	void Update(char* keys, char* preKeys) override {
		if (preKeys[DIK_SPACE] == 0 && keys[DIK_SPACE] != 0) {
			isEnd_ = true;
		}
	}

	void Draw() override {
		Novice::ScreenPrintf(520, 320, "SIMPLE_STG ");
		Novice::ScreenPrintf(460, 360, "PRESS SPACE TO START");
	}

	bool IsEnd() const override { return isEnd_; }
	std::unique_ptr<SceneBase> NextScene() override;

private:
	bool isEnd_ = false;
};

//--------------------------------
// GameScene
//--------------------------------
class GameScene : public SceneBase {
public:
	GameScene() {
		enemies_.resize(5);
		score_ = 0;
	}

	void Update(char* keys, char* preKeys) override {
		player_.Update(keys, preKeys);

		for (auto& enemy : enemies_) {
			enemy.Update();

			// プレイヤー被弾
			if (IsHit(player_.GetPos(), player_.GetRadius(),
				enemy.GetPos(), enemy.GetRadius())) {
				player_.Damage(100);
				enemy.Respawn();
			}

			// 弾ヒット
			for (auto& bullet : player_.GetBullets()) {
				if (bullet.IsActive() &&
					IsHit(bullet.GetPos(), bullet.GetRadius(),
						enemy.GetPos(), enemy.GetRadius())) {
					bullet.Deactivate();
					enemy.Respawn();
					score_ += 100;
				}
			}
		}

		if (score_ >= 5000) {
			state_ = 1; // clear
			isEnd_ = true;
		}
		if (player_.GetHP() <= 0) {
			state_ = 2; // gameover
			isEnd_ = true;
		}
	}

	void Draw() override {
		player_.Draw();
		for (const auto& enemy : enemies_) {
			enemy.Draw();
		}

		Novice::ScreenPrintf(20, 20, "SCORE: %d", score_);
		Novice::ScreenPrintf(20, 40, "HP: %d", player_.GetHP());
	}

	bool IsEnd() const override { return isEnd_; }
	std::unique_ptr<SceneBase> NextScene() override;

	int GetState() const { return state_; }

private:
	Player player_;
	std::vector<Enemy> enemies_;
	int score_;
	bool isEnd_ = false;
	int state_ = 0; // 1: clear, 2: gameover
};

//--------------------------------
// ClearScene
//--------------------------------
class ClearScene : public SceneBase {
public:
	void Update(char* keys, char* preKeys) override {
		if (preKeys[DIK_SPACE] == 0 && keys[DIK_SPACE] != 0) {
			isEnd_ = true;
		}
	}
	void Draw() override {
		Novice::ScreenPrintf(520, 320, "GAME CLEAR!");
		Novice::ScreenPrintf(420, 360, "PRESS SPACE TO TITLE");
	}
	bool IsEnd() const override { return isEnd_; }
	std::unique_ptr<SceneBase> NextScene() override {
		return std::make_unique<TitleScene>();
	}
private:
	bool isEnd_ = false;
};

//--------------------------------
// GameOverScene
//--------------------------------
class GameOverScene : public SceneBase {
public:
	void Update(char* keys, char* preKeys) override {
		if (preKeys[DIK_SPACE] == 0 && keys[DIK_SPACE] != 0) {
			isEnd_ = true;
		}
	}
	void Draw() override {
		Novice::ScreenPrintf(520, 320, "GAME OVER");
		Novice::ScreenPrintf(420, 360, "PRESS SPACE TO TITLE");
	}
	bool IsEnd() const override { return isEnd_; }
	std::unique_ptr<SceneBase> NextScene() override {
		return std::make_unique<TitleScene>();
	}
private:
	bool isEnd_ = false;
};

//--------------------------------
// Scene遷移
//--------------------------------
std::unique_ptr<SceneBase> TitleScene::NextScene() {
	return std::make_unique<GameScene>();
}

std::unique_ptr<SceneBase> GameScene::NextScene() {
	if (state_ == 1) {
		return std::make_unique<ClearScene>();
	} else {
		return std::make_unique<GameOverScene>();
	}
}

//--------------------------------
// main
//--------------------------------
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	Novice::Initialize(kWindowTitle, kWindowWidth, kWindowHeight);

	char keys[256] = { 0 };
	char preKeys[256] = { 0 };

	std::unique_ptr<SceneBase> scene = std::make_unique<TitleScene>();

	while (Novice::ProcessMessage() == 0) {
		Novice::BeginFrame();

		memcpy(preKeys, keys, 256);
		Novice::GetHitKeyStateAll(keys);

		scene->Update(keys, preKeys);
		scene->Draw();

		if (scene->IsEnd()) {
			scene = scene->NextScene();
		}

		Novice::EndFrame();

		if (keys[DIK_ESCAPE]) break;
	}

	Novice::Finalize();
	return 0;
}
