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
 
