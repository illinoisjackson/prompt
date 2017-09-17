echo "Downloading code..."
curl https://raw.githubusercontent.com/illinoisjackson/prompt/master/powerline.cpp -o pl.cpp
curl https://raw.githubusercontent.com/illinoisjackson/prompt/master/append.zsh -o append.zsh
echo "Compiling..."
g++ pl.cpp -O3 -pedantic -Wall --std=c++11 -o pl
mv pl ~/.powerline
