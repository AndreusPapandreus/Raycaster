run:
	clang++ -std=c++20 main.cpp ./client/client.cpp ./map/map.cpp ./entity/entity.cpp ./player/player.cpp ./enemy/enemy.cpp -o app -I /opt/homebrew/Cellar/glfw/3.4/include -I /opt/homebrew/Cellar/glm/1.0.1/include -I /opt/homebrew/opt/sfml/include -L /opt/homebrew/Cellar/glfw/3.4/lib -L /opt/homebrew/Cellar/glm/1.0.1/lib -L /opt/homebrew/opt/sfml/lib -lglfw -lsfml-system -lsfml-network -framework OpenGL && ./app

test:
	clang++ -std=c++20 main.cpp ./client/client.cpp ./map/map.cpp ./entity/entity.cpp ./player/player.cpp ./enemy/enemy.cpp -I/opt/homebrew/Cellar/glfw/3.4/include -I/opt/homebrew/opt/sfml/include -I/opt/homebrew/Cellar/glm/1.0.1/include /opt/homebrew/Cellar/glfw/3.4/lib/libglfw3.a /Users/andrij/Desktop/SFML/lib/libsfml-network-s.a /Users/andrij/Desktop/SFML/lib/libsfml-system-s.a -framework Foundation -framework CoreVideo -framework IOKit -framework Cocoa -framework OpenGL -o app -static-libstdc++ && ./app

server:
	clang++ -std=c++20 server.cpp -I/opt/homebrew/opt/sfml/include -o server -L/opt/homebrew/opt/sfml/lib -lsfml-system -lsfml-network && ./server
