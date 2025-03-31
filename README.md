# Trần Đức Hoàng - 24020141
- Demo game: https://www.youtube.com/watch?v=vvh7Yv2Qj48&t=141s

# Dodge And Q
"Dodge And Q" là một trò chơi hành động nhịp độ nhanh được phát triển bằng SDL2, nơi người chơi điều khiển một nhân vật chiến đấu chống lại làn sóng kẻ thù bằng cách né tránh và bắn đạn. Trò chơi có hệ thống nâng cấp, nhiều loại kẻ thù, và các cấp độ tăng dần độ khó, mang lại trải nghiệm hấp dẫn và thử thách.

# Giới thiệu trò chơi

Trong "Dodge And Q", bạn sẽ:
- Điều khiển nhân vật di chuyển bằng WASD và bắn đạn bằng phím Q.
- Né tránh các cuộc tấn công từ kẻ thù và tiêu diệt chúng để ghi điểm.
- Thu thập điểm số và combo để mở khóa các nâng cấp như tăng tốc độ, giảm thời gian hồi chiêu, tăng sát thương, hoặc mở khóa súng ngắn (Shotgun).
- Đối mặt với ba loại kẻ thù: Basic, Fast, và Chaser, mỗi loại có hành vi và tốc độ riêng biệt.
- Trải qua các cấp độ với độ khó tăng dần, thay đổi bản đồ ngẫu nhiên sau mỗi lần lên cấp.
- Trò chơi có giao diện người dùng trực quan, hiệu ứng âm thanh sống động, và hoạt ảnh mượt mà, tất cả được xây dựng bằng các thư viện SDL2, SDL_ttf, SDL_image, và SDL_mixer.

 # Tính năng chính:
- Điều khiển nhân vật: Di chuyển bằng WASD, bắn bằng phím Q.
- Hệ thống vũ khí: Chọn giữa bắn đơn (Single) hoặc súng ngắn (Shotgun) sau khi nâng cấp.

+ Kẻ thù đa dạng:
- Basic: Tốc độ trung bình, tấn công cận chiến.
- Fast: Di chuyển nhanh hơn, tấn công nhanh.
- Chaser: Máu cao hơn, bám đuổi người chơi.

+ Hệ thống nâng cấp: Tăng tốc độ, giảm thời gian hồi chiêu, tăng sát thương, hoặc mở khóa Shotgun.
+ Cấp độ và bản đồ: Độ khó tăng theo thời gian, với 4 bản đồ thay đổi ngẫu nhiên mỗi khi lên cấp.
+ Combo và điểm số: Tiêu diệt kẻ thù liên tiếp để tăng combo và nhân điểm.
+ Âm thanh và hình ảnh: Nhạc nền, hiệu ứng âm thanh khi bắn, trúng đòn, và chết; hoạt ảnh chi tiết cho nhân vật và kẻ thù.
+ Menu và cài đặt: Menu chính với các tùy chọn chơi, cài đặt âm lượng nhạc và hiệu ứng, cùng nút thoát.

# Cách chơi
- Mục tiêu: Sống sót qua các cấp độ, tiêu diệt kẻ thù, và đạt điểm cao nhất có thể.
- Điều khiển:
+ WASD: Di chuyển nhân vật.
+ Q: Bắn đạn (hoặc Shotgun nếu đã mở khóa).
+ Esc: Tạm dừng trò chơi.

# Cơ chế:
- Mỗi cấp độ kéo dài 30 giây, sau đó bạn lên cấp và có cơ hội nâng cấp.
- Kẻ thù xuất hiện ngẫu nhiên với tần suất tăng dần.
- Khi hết máu (100 HP), trò chơi kết thúc và bạn có thể chơi lại.
- Nâng cấp: Chọn một trong bốn tùy chọn khi lên cấp bằng phím số 1-4.

 # Game info

- Start menu
![image](https://github.com/user-attachments/assets/7833abf3-fece-4e1f-be87-59830942bf7f)

- Settings menu
![image](https://github.com/user-attachments/assets/74a5fc6d-133a-4e9e-84f8-6db7589ce245)

- Playing game
![image](https://github.com/user-attachments/assets/6f134693-7492-42af-855a-3d4c07a1e57d)

- Level up notification
![image](https://github.com/user-attachments/assets/85614033-9f4f-4822-89a6-af8120627546)


- Upgrade menu
![image](https://github.com/user-attachments/assets/fd4e0d06-435b-421d-9e18-834f8447dfd4)


- Game over menu
![image](https://github.com/user-attachments/assets/41a07610-05de-4282-adc5-da6d955a3a98)

- Paused menu ( bấm nút ESC )
![image](https://github.com/user-attachments/assets/9ca13969-87c9-4f7a-bde0-d78bba98b051)

# Code game
1. Vòng lặp chính (Main Loop)
- Thuật toán: Vòng lặp vô hạn chạy cho đến khi người chơi thoát (running = false).
+ Bước 1: Tính deltaTime (thời gian giữa hai khung hình) để đảm bảo chuyển động mượt mà, độc lập với tốc độ khung hình.
+ Bước 2: Xử lý sự kiện (event handling) từ bàn phím và chuột.
+ Bước 3: Cập nhật trạng thái game (update) dựa trên gameState.
+ Bước 4: Vẽ toàn bộ giao diện và vật thể lên màn hình (render).
- Ý nghĩa: Điều phối toàn bộ logic game, đảm bảo tính thời gian thực.
2. Quản lý trạng thái game (Game State Management)
- Thuật toán: Sử dụng enum GameState (MENU, PLAYING, PAUSED, LEVEL_UP, v.v.) để chuyển đổi giữa các màn hình và chế độ chơi.
- Switch-case trong update và render để xử lý logic và hiển thị riêng cho từng trạng thái.
- Ví dụ: Trong PLAYING, cập nhật nhân vật, kẻ thù, đạn; trong UPGRADE_MENU, hiển thị và xử lý lựa chọn nâng cấp.
- Ý nghĩa: Tổ chức game thành các giai đoạn rõ ràng, dễ mở rộng.
3. Cập nhật nhân vật (Player Update)
- Thuật toán:
+ Đọc input từ phím WASD để tính vận tốc (vx, vy) dựa trên PLAYER_SPEED và deltaTime.
+ Chuẩn hóa vận tốc để tránh di chuyển nhanh hơn khi đi chéo.
+ Cập nhật vị trí (rect.x, rect.y) và hitbox, kiểm tra biên màn hình.
+ Chuyển đổi trạng thái hoạt ảnh (IDLE, WALK, ATTACK, HURT, DEAD) dựa trên hành động.
+ Xử lý bắn đạn khi nhấn Q (gọi shootProjectile hoặc shootShotgun).
- Ý nghĩa: Điều khiển mượt mà, phản hồi tức thì với input người chơi.
4. Cập nhật kẻ thù (Enemy Update)
-Thuật toán:
+ Duyệt qua danh sách enemies:
* Tính khoảng cách đến người chơi (dx, dy), chuẩn hóa để di chuyển với tốc độ ENEMY_SPEED.
* Nếu trong tầm SLASHING_DISTANCE, chuyển sang trạng thái SLASHING và tấn công.
* Nếu không, di chuyển theo hướng người chơi với tốc độ khác nhau tùy loại (BASIC, FAST, CHASER).
* Cập nhật hoạt ảnh (walking, slashing, dying) và giảm máu khi trúng đạn.
* Khi chết (health <= 0), chuyển sang DYING, tăng điểm và combo.
* Xóa kẻ thù không hoạt động (active = false).
* Ý nghĩa: Tạo hành vi kẻ thù thông minh, tăng độ khó qua cấp độ.
5. Cập nhật đạn (Projectile Update)
- Thuật toán:
+ Duyệt qua projectiles:
* Di chuyển đạn theo vận tốc (vx, vy) tính từ hướng bắn.
* Kiểm tra va chạm với hitbox của kẻ thù, giảm máu kẻ thù và vô hiệu hóa đạn.
* Xóa đạn nếu ra ngoài màn hình.
* Quản lý thời gian hồi chiêu (qCooldown) để giới hạn tần suất bắn.
* Ý nghĩa: Xử lý cơ chế tấn công chính của người chơi.
6. Quản lý cấp độ và nâng cấp (Leveling and Upgrades)
- Thuật toán:
+ Theo dõi gameTime, khi vượt LEVEL_DURATION * level, chuyển sang PRE_LEVEL_UP.
+ Sau PRE_LEVEL_UP_DELAY, vào LEVEL_UP: tăng level, thêm upgradePoints, giảm spawnRate.
+ Trong UPGRADE_MENU, xử lý lựa chọn nâng cấp (phím 1-4):
+ Tăng playerSpeed, giảm qCooldownMax, tăng projectileDamage, hoặc mở SHOTGUN.
+ Đổi bản đồ ngẫu nhiên và sinh kẻ thù mới.
+ Ý nghĩa: Tăng tính thử thách và cho phép tùy chỉnh lối chơi.
7. Xử lý va chạm và sát thương
- Thuật toán:
+ Kiểm tra va chạm giữa hitbox của người chơi và kẻ thù khi kẻ thù ở trạng thái SLASHING.
+ Giảm player.health và đặt lại attackCooldown của kẻ thù.
+ Nếu health <= 0, chuyển sang DEAD, sau đó GAME_OVER.
+ Kiểm tra va chạm giữa đạn và kẻ thù để gây sát thương.
+ Ý nghĩa: Đảm bảo tương tác vật lý chính xác, tạo cảm giác nguy hiểm.
8. Sinh kẻ thù (Enemy Spawning)
- Thuật toán:
+ Ngẫu nhiên tạo Marker cách người chơi tối thiểu MIN_SPAWN_DISTANCE.
+ Sau SPAWN_DELAY, sinh kẻ thù tại vị trí Marker với loại ngẫu nhiên (BASIC, FAST, CHASER).
+ Tần suất sinh giảm dần theo spawnRate khi lên cấp.
+ Ý nghĩa: Điều chỉnh độ khó động dựa trên tiến trình game.


8. Tổng kết
- Tính toán thời gian thực: Sử dụng deltaTime để đồng bộ hóa chuyển động và hoạt ảnh.
- Quản lý danh sách động: Vector cho kẻ thù, đạn, particle, với xóa phần tử không hoạt động bằng erase-remove idiom.
- Hành vi AI đơn giản: Kẻ thù bám đuổi hoặc tấn công dựa trên khoảng cách.
- Tối ưu hóa: Hitbox nhỏ hơn hình ảnh để va chạm chính xác hơn.







 
