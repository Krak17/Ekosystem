#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <tuple>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <memory>
#include <map>
#include <fstream>

// Struktura do reprezentowania efektów czasowych (np. zatrucia)
struct Effect {
    int damagePerTurn;
    int duration;
};

// Klasa bazowa dla zwierząt
class Animal {
public:
    Animal(std::string type, char symbol, int strength, int health, int speed)
        : type(type), symbol(symbol), baseStrength(strength), strength(strength), health(health), baseSpeed(speed), speed(speed), position(-1, -1), previousPosition(-1, -1) {}

    // Funkcja wyświetlająca informacje o zwierzęciu
    void display() const {
        std::cout << "Typ: " << type << ", Symbol: " << symbol << ", Sila: " << strength
            << ", Zdrowie: " << health << ", Predkosc: " << speed << std::endl;
    }

    char getSymbol() const {
        return symbol;
    }

    std::string getType() const {
        return type;
    }

    std::tuple<int, int> getPosition() const {
        return position;
    }

    void setPosition(int x, int y) {
        previousPosition = position;
        position = std::make_tuple(x, y);
    }

    // Funkcja odpowiedzialna za ruch zwierzęcia
    virtual void move(std::vector<std::vector<char>>& board, int steps) {
        int x, y;
        std::tie(x, y) = position;
        std::mt19937 rng(std::random_device{}());

        for (int step = 0; step < steps; ++step) {
            std::vector<std::tuple<int, int>> possibleMoves = {
                {x - 1, y}, {x + 1, y}, {x, y - 1}, {x, y + 1}
            };
            std::shuffle(possibleMoves.begin(), possibleMoves.end(), rng);
            bool moved = false;
            for (const auto& move : possibleMoves) {
                int newX = std::get<0>(move);
                int newY = std::get<1>(move);
                if (newX >= 0 && newX < board.size() && newY >= 0 && newY < board[0].size() &&
                    (board[newX][newY] == ' ' || board[newX][newY] == ',' || board[newX][newY] == '@' || board[newX][newY] == '*') &&
                    (newX != std::get<0>(previousPosition) || newY != std::get<1>(previousPosition))) {
                    if (board[newX][newY] != ' ') {
                        consumeResource(board[newX][newY]);
                    }
                    board[x][y] = ' ';
                    board[newX][newY] = symbol;
                    setPosition(newX, newY);
                    x = newX;
                    y = newY;
                    moved = true;
                    break;
                }
            }
            if (!moved) {
                break; // Jeśli ruch jest niemożliwy, przerwij pętlę
            }
        }
    }

    int getHealth() const {
        return health;
    }

    void modifyHealth(int delta) {
        health += delta;
    }

    // Funkcja odpowiedzialna za konsumowanie zasobów przez zwierzę
    void consumeResource(char resourceSymbol) {
        if (resourceSymbol == ',') {
            health += 1;
        }
        else if (resourceSymbol == '@') {
            activeBonuses["predkosc"] = { 2, 3 }; // +2 do szybkości na 3 tury
        }
        else if (resourceSymbol == '*') {
            activeBonuses["sila"] = { 5, 3 }; // +5 do siły na 3 tury
            health += 5;
        }
    }

    int getSpeed() const {
        return speed;
    }

    virtual void useUniqueAbility() {}

    virtual void hunt(std::vector<std::vector<char>>& board, std::vector<std::shared_ptr<Animal>>& prey) {}

    // Funkcja do aktualizacji bonusów
    void updateBonuses() {
        speed = baseSpeed; // Resetuj szybkość do bazowej wartości
        strength = baseStrength; // Resetuj siłę do bazowej wartości
        for (auto it = activeBonuses.begin(); it != activeBonuses.end(); ) {
            if (it->first == "predkosc") {
                speed += it->second.first;
            }
            else if (it->first == "sila") {
                strength += it->second.first;
            }
            it->second.second--; // Zmniejsz czas trwania bonusu
            if (it->second.second <= 0) {
                it = activeBonuses.erase(it); // Usuń wygasłe bonusy
            }
            else {
                ++it;
            }
        }
    }

    // Funkcja do stosowania efektów (np. trucizna)
    void applyEffects() {
        for (auto it = activeEffects.begin(); it != activeEffects.end(); ) {
            modifyHealth(-it->damagePerTurn);
            it->duration--;
            if (it->duration <= 0) {
                it = activeEffects.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    void addEffect(const Effect& effect) {
        activeEffects.push_back(effect);
    }

protected:
    std::string type;
    char symbol;
    int baseStrength;
    int strength;
    int health;
    int baseSpeed;
    int speed;
    std::tuple<int, int> position;
    std::tuple<int, int> previousPosition;
    std::map<std::string, std::pair<int, int>> activeBonuses; // nazwa bonusu -> (wartość, czas trwania)
    std::vector<Effect> activeEffects;
};

// Klasa dla drapieżników
class Carnivore : public Animal {
public:
    Carnivore(std::string type, char symbol, int strength, int health, int speed)
        : Animal(type, symbol, strength, health, speed) {}

    void hunt(std::vector<std::vector<char>>& board, std::vector<std::shared_ptr<Animal>>& animals) override {
        int x, y;
        std::tie(x, y) = getPosition();

        std::vector<std::tuple<int, int>> adjacentPositions = {
            {x - 1, y}, {x + 1, y}, {x, y - 1}, {x, y + 1}
        };

        for (const auto& pos : adjacentPositions) {
            int ax = std::get<0>(pos);
            int ay = std::get<1>(pos);

            if (ax >= 0 && ax < board.size() && ay >= 0 && ay < board[0].size()) {
                auto it = std::find_if(animals.begin(), animals.end(), [ax, ay](const std::shared_ptr<Animal>& animal) {
                    int animalX, animalY;
                    std::tie(animalX, animalY) = animal->getPosition();
                    return animalX == ax && animalY == ay;
                    });

                if (it != animals.end() && (*it)->getType() != this->getType()) {
                    (*it)->modifyHealth(-strength);
                    if ((*it)->getHealth() <= 0) {
                        board[ax][ay] = ' ';
                        animals.erase(it);
                    }
                }
            }
        }
    }

    void useUniqueAbility() override {}
};

// Klasa dla roślinożerców
class Herbivore : public Animal {
public:
    Herbivore(std::string type, char symbol, int strength, int health, int speed)
        : Animal(type, symbol, strength, health, speed) {}

    void useUniqueAbility() override {}
};

// Definicje klas dla poszczególnych zwierząt z unikalnymi zdolnościami
class Wolf : public Carnivore {
public:
    Wolf() : Carnivore("Wilk", 'W', 30, 60, 7) {}
};

class Eagle : public Carnivore {
public:
    Eagle() : Carnivore("Orzel", 'E', 20, 30, 9) {}

    // Polowanie dla Orła z większym zasięgiem
    void hunt(std::vector<std::vector<char>>& board, std::vector<std::shared_ptr<Animal>>& animals) override {
        int x, y;
        std::tie(x, y) = getPosition();

        std::vector<std::tuple<int, int>> extendedPositions = {
            {x - 2, y}, {x + 2, y}, {x, y - 2}, {x, y + 2},
            {x - 1, y}, {x + 1, y}, {x, y - 1}, {x, y + 1}
        };

        for (const auto& pos : extendedPositions) {
            int ax = std::get<0>(pos);
            int ay = std::get<1>(pos);

            if (ax >= 0 && ax < board.size() && ay >= 0 && ay < board[0].size()) {
                auto it = std::find_if(animals.begin(), animals.end(), [ax, ay](const std::shared_ptr<Animal>& animal) {
                    int animalX, animalY;
                    std::tie(animalX, animalY) = animal->getPosition();
                    return animalX == ax && animalY == ay;
                    });

                if (it != animals.end() && (*it)->getType() != this->getType()) {
                    (*it)->modifyHealth(-strength);
                    if ((*it)->getHealth() <= 0) {
                        board[ax][ay] = ' ';
                        animals.erase(it);
                    }
                }
            }
        }
    }
};

class Viper : public Carnivore {
public:
    Viper() : Carnivore("Zmija", 'V', 15, 40, 5) {}

    // Polowanie dla Żmiji - trutka
    void hunt(std::vector<std::vector<char>>& board, std::vector<std::shared_ptr<Animal>>& animals) override {
        int x, y;
        std::tie(x, y) = getPosition();

        std::vector<std::tuple<int, int>> adjacentPositions = {
            {x - 1, y}, {x + 1, y}, {x, y - 1}, {x, y + 1}
        };

        for (const auto& pos : adjacentPositions) {
            int ax = std::get<0>(pos);
            int ay = std::get<1>(pos);

            if (ax >= 0 && ax < board.size() && ay >= 0 && ay < board[0].size()) {
                auto it = std::find_if(animals.begin(), animals.end(), [ax, ay](const std::shared_ptr<Animal>& animal) {
                    int animalX, animalY;
                    std::tie(animalX, animalY) = animal->getPosition();
                    return animalX == ax && animalY == ay;
                    });

                if (it != animals.end() && (*it)->getType() != this->getType()) {
                    (*it)->modifyHealth(-strength);
                    (*it)->addEffect({ 5, 3 }); // Dodaj efekt trucizny: 5 obrażeń co turę przez 3 tur
                    if ((*it)->getHealth() <= 0) {
                        board[ax][ay] = ' ';
                        animals.erase(it);
                    }
                }
            }
        }
    }
};

class Bear : public Carnivore {
public:
    Bear() : Carnivore("Niedzwiedz", 'B', 50, 80, 4) {}
};

class Bison : public Herbivore {
public:
    Bison() : Herbivore("Bizon", 'Z', 40, 100, 3) {}

};

class Sparrow : public Herbivore {
public:
    Sparrow() : Herbivore("Wrobel", 'S', 10, 15, 10) {}

    //Umiejętność - unik
    void useUniqueAbility() override {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 1.0);
        double chance = dis(gen); // Losujemy liczbę z przedziału [0, 1]

        if (chance <= 0.5) { // 50% szansy na uniknięcie ataku
            std::cout << "Wrobel uniknal ataku!" << std::endl;
            return;
        }
    }
};

class Hare : public Herbivore {
public:
    Hare() : Herbivore("Zajac", 'H', 15, 35, 7) {}

    //Umiejętność - unik
    void useUniqueAbility() override {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 1.0);
        double chance = dis(gen); // Losujemy liczbę z przedziału [0, 1]

        if (chance <= 0.3) { // 30% szansy na uniknięcie ataku
            std::cout << "Zajac uniknal ataku!" << std::endl;
            return;
        }
    }
};


class RoeDeer : public Herbivore {
public:
    RoeDeer() : Herbivore("Jelen", 'D', 25, 55, 8) {}

};

// Klasa dla zasobów
class Resource {
public:
    Resource(std::string name, char symbol)
        : name(name), symbol(symbol) {}

    void display() const {
        std::cout << name << ", Symbol: " << symbol << std::endl;
    }

private:
    std::string name;
    char symbol;
};

// Funkcja do wypełnienia planszy losowymi znakami
void fillBoard(std::vector<std::vector<char>>& board, int size) {
    int totalCells = size * size;
    int numGrass = totalCells * 60 / 100;
    int numBushWithFruit = totalCells * 10 / 100;
    int numMushroom = totalCells * 5 / 100;
    int numEmpty = totalCells * 25 / 100;

    std::vector<char> cells;
    cells.insert(cells.end(), numGrass, ',');
    cells.insert(cells.end(), numBushWithFruit, '@');
    cells.insert(cells.end(), numMushroom, '*');
    cells.insert(cells.end(), numEmpty, ' ');

    std::mt19937 rng(std::random_device{}());
    std::shuffle(cells.begin(), cells.end(), rng);

    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            board[i][j] = cells[i * size + j];
        }
    }
}

// Funkcja do losowego umieszczania zwierząt na planszy
void placeAnimals(std::vector<std::vector<char>>& board, std::vector<std::shared_ptr<Animal>>& animals, int countPerAnimal) {
    std::mt19937 rng(std::random_device{}());
    int size = board.size();

    for (int count = 0; count < countPerAnimal; ++count) {
        for (auto& animal : animals) {
            bool placed = false;
            while (!placed) {
                int i = rng() % size;
                int j = rng() % size;
                if (board[i][j] == ' ') {
                    board[i][j] = animal->getSymbol();
                    animal->setPosition(i, j);
                    placed = true;
                }
            }
        }
    }
}

// Funkcja do wypełnienia pustych miejsc znakami 'x'
void fillEmptySpacesWithStones(std::vector<std::vector<char>>& board) {
    bool placeStone = true; // Flaga określająca, czy powinien być stawiany kamień

    for (auto& row : board) {
        for (char& cell : row) {
            if (cell == ' ') {
                if (placeStone) {
                    cell = 'x';
                }
                placeStone = !placeStone; // Zmiana flagi dla następnego pola
            }
        }
    }
}

// Funkcja do wyświetlania planszy
void displayBoard(const std::vector<std::vector<char>>& board) {
    for (const auto& row : board) {
        for (char cell : row) {
            std::cout << cell << ' ';
        }
        std::cout << std::endl;
    }
}

// Fukcja do wybrania rozmiaru planszy przez użytkownika
int chooseBoardSize() {
    int size;
    std::cout << "Wybierz rozmiar planszy (zalecane pomiedzy 1 a 4): ";
    std::cin >> size;
    while (size < 1) {
        std::cout << "Rozmiar planszy musi wynosic co najmniej 1. Wybierz ponownie: ";
        std::cin >> size;
    }
    return 10 * size;
}

// Fukcja do wybrania ilości zwierząt przez użytkownika
int chooseAnimalCount() {
    int count;
    std::cout << "Wybierz liczbe zwierząt z każdego gatunku (minimum 1): ";
    std::cin >> count;
    while (count < 1) {
        std::cout << "Liczba zwierzat musi być co najmniej 1. Wybierz ponownie: ";
        std::cin >> count;
    }
    return count;
}

// Funkcja do wyświetlania ekranu początkowego
void mainTitle() {
    std::cout << "                           _       _                            " << std::endl;
    std::cout << " ___ _   _ _ __ ___  _   _| | __ _| |_ ___  _ __                " << std::endl;
    std::cout << "/ __| | | | '_ ` _ \\| | | | |/ _` | __/ _ \\| '__|               " << std::endl;
    std::cout << "\\__ \\ |_| | | | | | | |_| | | (_| | || (_) | |                  " << std::endl;
    std::cout << "|___/\\__, |_| |_| |_|\\__,_|_|\\__,_|\\__\\___/|_|                  " << std::endl;
    std::cout << "     |___/  ___| | _____  ___ _   _ ___| |_ ___ _ __ ___  _   _ " << std::endl;
    std::cout << "           / _ \\ |/ / _ \\/ __| | | / __| __/ _ \\ '_ ` _ \\| | | |" << std::endl;
    std::cout << "          |  __/   < (_) \\__ \\ |_| \\__ \\ ||  __/ | | | | | |_| |" << std::endl;
    std::cout << "           \\___|_|\\_\\___/|___/\\__, |___/\\__\\___|_| |_| |_|\\__,_|" << std::endl;
    std::cout << "                              |___/                             " << std::endl;
}

// Funkcja do czyszczenia konsoli
void clearConsole() {
#ifdef _WIN32
    system("CLS");
#else
    system("clear");
#endif
}

int main() {
    mainTitle();
    int boardSize = chooseBoardSize();
    int countPerAnimal = chooseAnimalCount();

    std::cout << "Wybrany rozmiar planszy: " << boardSize << std::endl;
    std::cout << "Wybrana liczba zwierząt z każdego gatunku: " << countPerAnimal << std::endl;

    std::vector<std::vector<char>> board(boardSize, std::vector<char>(boardSize));

    fillBoard(board, boardSize);

    std::vector<std::shared_ptr<Animal>> carnivores = {
        std::make_shared<Wolf>(), std::make_shared<Eagle>(), std::make_shared<Viper>(), std::make_shared<Bear>()
    };
    std::vector<std::shared_ptr<Animal>> herbivores = {
        std::make_shared<Bison>(), std::make_shared<Sparrow>(), std::make_shared<Hare>(), std::make_shared<RoeDeer>()
    };
    std::vector<std::shared_ptr<Animal>> animals;
    animals.insert(animals.end(), carnivores.begin(), carnivores.end());
    animals.insert(animals.end(), herbivores.begin(), herbivores.end());

    placeAnimals(board, animals, countPerAnimal);

    fillEmptySpacesWithStones(board);

    Resource grass("Trawa", ',');
    Resource bushWithFruit("Krzak z owoca", '@');
    Resource mushroom("Grzyb", '*');

    int turnCounter = 0;

    while (true) {
        clearConsole();
        std::cout << "Tura: " << turnCounter << std::endl;

        displayBoard(board);

        std::cout << "\nZwierzeta:" << std::endl;
        for (const auto& animal : animals) {
            int x, y;
            std::tie(x, y) = animal->getPosition();
            std::cout << "Pozycja: (" << x << ", " << y << "), ";
            animal->display();
        }

        std::cout << "\nZasoby:" << std::endl;
        grass.display();
        bushWithFruit.display();
        mushroom.display();

        for (auto& animal : animals) {
            animal->updateBonuses();
            animal->move(board, animal->getSpeed());
            animal->consumeResource(board[std::get<0>(animal->getPosition())][std::get<1>(animal->getPosition())]);
            animal->useUniqueAbility();
            animal->applyEffects();
        }
        for (auto& carnivore : carnivores) {
            carnivore->hunt(board, animals);
        }

        // Sprawdzenie liczby gatunków na planszy
        std::map<std::string, int> speciesCount;
        for (const auto& animal : animals) {
            speciesCount[animal->getType()]++;
        }
        if (speciesCount.size() == 1) {
            std::string lastSpecies = speciesCount.begin()->first;
            int rounds = turnCounter;

            // Zapis wyniku do pliku CSV
            std::ofstream outputFile("simulation_result.csv", std::ios_base::app);
            if (outputFile.is_open()) {
                outputFile << lastSpecies << "," << rounds << std::endl;
                outputFile.close();
            }
            else {
                std::cerr << "Błąd przy zapisie do pliku CSV!" << std::endl;
            }

            std::cout << "Ostatnim gatunkiem na planszy jest: " << lastSpecies << ". Liczba wykonanych rund: " << rounds << std::endl;
            break;
        }

        turnCounter++;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}