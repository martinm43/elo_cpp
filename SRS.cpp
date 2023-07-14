#include <iostream>
#include <vector>

struct Team {
    std::string name;
    int pointsScored;
    int pointsAllowed;
    double rating;
};

double calculateSRS(const std::vector<Team>& teams) {
    // Step 1: Calculate the average point differential (PD)
    double totalPD = 0.0;
    for (const Team& team : teams) {
        totalPD += team.pointsScored - team.pointsAllowed;
    }
    double avgPD = totalPD / teams.size();

    // Step 2: Calculate the team's SRS
    for (Team& team : teams) {
        team.rating = (team.pointsScored - team.pointsAllowed) - avgPD;
    }

    // Step 3: Calculate the league average SRS
    double totalSRS = 0.0;
    for (const Team& team : teams) {
        totalSRS += team.rating;
    }
    double leagueAvgSRS = totalSRS / teams.size();

    // Step 4: Adjust the ratings to maintain a zero average
    for (Team& team : teams) {
        team.rating -= leagueAvgSRS;
    }

    // Return the SRS value for the first team in the vector
    return teams[0].rating;
}

int main() {
    // Sample data for 4 teams
    std::vector<Team> teams = {
        {"Team A", 100, 80, 0.0},
        {"Team B", 90, 70, 0.0},
        {"Team C", 80, 90, 0.0},
        {"Team D", 70, 100, 0.0}
    };

    double srs = calculateSRS(teams);
    std::cout << "SRS for " << teams[0].name << ": " << srs << std::endl;

    return 0;
}

