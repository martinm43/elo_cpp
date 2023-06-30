#include <math.h>
#include <iostream>
#include <sqlite3.h>
#include <vector>

#define KFACTOR 30
#define NUM_TEAMS 30
#define BASE_RATING 1500.0

//ORM
struct Game {
    int id;
    int home_team_id;
    int home_team_runs;
    int away_team_id;
    int away_team_runs;
    int year;
    double epochtime;
};

// Function to calculate the expected probability of player A winning
double calculateExpectedProbability(int ratingA, int ratingB) {
    return 1.0 / (1.0 + pow(10.0, (ratingB - ratingA) / 400.0));
}

// Function to update the Elo rating of a player
int updateEloRating(int rating, double score, double expectedScore, int kFactor) {
    return rating + kFactor * (score - expectedScore);
}

int selectDataCallback(void* data, int argc, char** argv, char** /*azColName*/) {
    std::vector<Game>* games = static_cast<std::vector<Game>*>(data);

    Game game;
    game.id = std::stoi(argv[0]);
    game.home_team_id = std::stoi(argv[1]);
    game.home_team_runs = std::stoi(argv[2]);
    game.away_team_id = std::stoi(argv[3]);
    game.away_team_runs = std::stoi(argv[4]);
    game.year = std::stoi(argv[5]);
    game.epochtime = std::stod(argv[6]);
    games->push_back(game);

    return 0;
}

int main() {
    sqlite3* db;
    char* errorMsg = nullptr;

    // Open a connection to the database
    int rc = sqlite3_open("mlb_data.sqlite", &db);
    if (rc != SQLITE_OK) {
        std::cout << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return rc;
    }

    // Query the table
    const char* selectDataQuery = "SELECT id, home_team_id, home_team_runs, away_team_id, away_team_runs, year , epochtime FROM Games WHERE year=2015";
    std::vector<Game> games;
    rc = sqlite3_exec(db, selectDataQuery, selectDataCallback, &games, &errorMsg);
    if (rc != SQLITE_OK) {
        std::cout << "SQL error: " << errorMsg << std::endl;
        sqlite3_free(errorMsg);
        sqlite3_close(db);
        return rc;
    }

    // Close the database connection
    sqlite3_close(db);

    // Display the retrieved data
    for (const auto& game : games) {
        std::cout << "ID: " << game.id << " Year: " << game.year << std:: endl ; 
    	//<< ", Name: " << game.name << ", Score: " << game.score << std::endl;
    }

    std::cout << "Total size of the table: " << games.size() << std::endl;

    std::vector<double> eloratings(NUM_TEAMS, BASE_RATING);

    return 0;
}
