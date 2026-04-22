#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>

using namespace std;

// ============================================================
// Data structures
// ============================================================

struct SubRecord {
    string problem;
    string status;
    int time;
};

struct ProblemState {
    int wrong_attempts = 0;
    int solved_time = -1;
    int wrong_before_ac = 0;
    bool solved_before_freeze = false;
    int frozen_wrong_before = 0;
    int frozen_attempts = 0;
    bool frozen_has_ac = false;
    int frozen_ac_time = -1;
    bool is_frozen = false;
};

struct Team {
    string name;
    vector<ProblemState> probs;
    int solved = 0;
    int penalty = 0;
    vector<int> solve_times;
    vector<SubRecord> subs;
};

// ============================================================
// Globals
// ============================================================

bool started = false;
bool frozen_state = false;
int duration = 0;
int M = 0;
vector<Team> teams;
map<string, int> team_idx;
vector<int> ranking;
bool ranking_valid = false;

// ============================================================
// Helpers
// ============================================================

int pidx(const string& name) { return name[0] - 'A'; }
bool is_ac(const string& s) { return s == "Accepted"; }

// ============================================================
// Ranking comparison
// ============================================================

bool cmp_rank(int a, int b) {
    const Team& ta = teams[a];
    const Team& tb = teams[b];
    if (ta.solved != tb.solved) return ta.solved > tb.solved;
    if (ta.penalty != tb.penalty) return ta.penalty < tb.penalty;
    int i = (int)ta.solve_times.size() - 1;
    int j = (int)tb.solve_times.size() - 1;
    while (i >= 0 && j >= 0) {
        if (ta.solve_times[i] != tb.solve_times[j])
            return ta.solve_times[i] < tb.solve_times[j];
        i--; j--;
    }
    if (j >= 0) return true;
    if (i >= 0) return false;
    return ta.name < tb.name;
}

void flush_ranking() {
    if (ranking_valid) return;
    ranking.resize(teams.size());
    for (int i = 0; i < (int)teams.size(); i++) ranking[i] = i;
    sort(ranking.begin(), ranking.end(), cmp_rank);
    ranking_valid = true;
}

int get_rank(const string& name) {
    auto it = team_idx.find(name);
    if (it == team_idx.end()) return -1;
    int idx = it->second;
    for (int i = 0; i < (int)ranking.size(); i++)
        if (ranking[i] == idx) return i + 1;
    return -1;
}

// ============================================================
// Problem display
// ============================================================

string prob_str(const Team& team, int p, bool show_frozen) {
    const ProblemState& ps = team.probs[p];
    if (show_frozen && ps.is_frozen && !ps.solved_before_freeze) {
        int x = ps.frozen_wrong_before;
        if (x > 0) return to_string(x) + "/" + to_string(ps.frozen_attempts);
        return "0/" + to_string(ps.frozen_attempts);
    }
    if (ps.solved_time != -1) {
        if (ps.wrong_before_ac == 0) return "+";
        return "+" + to_string(ps.wrong_before_ac);
    }
    int total = ps.wrong_attempts + ps.frozen_attempts;
    if (total == 0) return ".";
    return "-" + to_string(total);
}

// ============================================================
// Freeze
// ============================================================

void do_freeze() {
    for (auto& team : teams) {
        for (int p = 0; p < M; p++) {
            ProblemState& ps = team.probs[p];
            if (ps.solved_time != -1) {
                ps.solved_before_freeze = true;
                ps.is_frozen = false;
            } else {
                ps.solved_before_freeze = false;
                ps.frozen_wrong_before = ps.wrong_attempts;
                ps.is_frozen = (ps.frozen_attempts > 0);
            }
        }
    }
}

// ============================================================
// Scroll
// ============================================================

void do_scroll() {
    cout << "[Info]Scroll scoreboard.\n";

    flush_ranking();

    // Output scoreboard before scrolling
    for (int r = 0; r < (int)ranking.size(); r++) {
        int ti = ranking[r];
        Team& team = teams[ti];
        cout << team.name << " " << (r+1) << " " << team.solved << " " << team.penalty;
        for (int p = 0; p < M; p++)
            cout << " " << prob_str(team, p, true);
        cout << "\n";
    }

    // Build frozen problem list per team
    vector<vector<int>> frozen_list(teams.size());
    for (int ti = 0; ti < (int)teams.size(); ti++) {
        for (int p = 0; p < M; p++) {
            if (teams[ti].probs[p].is_frozen)
                frozen_list[ti].push_back(p);
        }
    }

    // Scroll loop
    while (true) {
        int lowest_ti = -1, lowest_r = -1;
        for (int r = (int)ranking.size() - 1; r >= 0; r--) {
            int ti = ranking[r];
            if (!frozen_list[ti].empty()) {
                lowest_ti = ti;
                lowest_r = r;
                break;
            }
        }
        if (lowest_ti == -1) break;

        int p = frozen_list[lowest_ti].front();
        frozen_list[lowest_ti].erase(frozen_list[lowest_ti].begin());

        Team& team = teams[lowest_ti];
        ProblemState& ps = team.probs[p];
        ps.is_frozen = false;

        if (ps.frozen_has_ac) {
            team.solved++;
            team.penalty += 20 * ps.frozen_wrong_before + ps.frozen_ac_time;
            auto it = lower_bound(team.solve_times.begin(), team.solve_times.end(), ps.frozen_ac_time);
            team.solve_times.insert(it, ps.frozen_ac_time);
            ps.wrong_before_ac = ps.frozen_wrong_before;
            ps.solved_time = ps.frozen_ac_time;
        }

        // Bubble the team up to its correct position
        int cur_r = lowest_r;
        while (cur_r > 0 && cmp_rank(ranking[cur_r], ranking[cur_r - 1])) {
            swap(ranking[cur_r], ranking[cur_r - 1]);
            cur_r--;
        }

        if (cur_r < lowest_r) {
            int displaced_ti = ranking[lowest_r];
            cout << team.name << " " << teams[displaced_ti].name << " "
                 << team.solved << " " << team.penalty << "\n";
        }
    }

    // Output scoreboard after scrolling
    for (int r = 0; r < (int)ranking.size(); r++) {
        int ti = ranking[r];
        Team& team = teams[ti];
        cout << team.name << " " << (r+1) << " " << team.solved << " " << team.penalty;
        for (int p = 0; p < M; p++)
            cout << " " << prob_str(team, p, false);
        cout << "\n";
    }
}

// ============================================================
// Main
// ============================================================

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string line;
    while (getline(cin, line)) {
        if (line.empty()) continue;
        istringstream iss(line);
        string cmd;
        iss >> cmd;

        if (cmd == "ADDTEAM") {
            string name;
            iss >> name;
            if (started) {
                cout << "[Error]Add failed: competition has started.\n";
            } else if (team_idx.count(name)) {
                cout << "[Error]Add failed: duplicated team name.\n";
            } else {
                team_idx[name] = (int)teams.size();
                teams.push_back({name});
                cout << "[Info]Add successfully.\n";
            }
        }
        else if (cmd == "START") {
            string dummy;
            int new_duration, new_M;
            iss >> dummy >> new_duration >> dummy >> new_M;
            if (started) {
                cout << "[Error]Start failed: competition has started.\n";
            } else {
                started = true;
                duration = new_duration;
                M = new_M;
                for (auto& t : teams) t.probs.resize(M);
                cout << "[Info]Competition starts.\n";
            }
        }
        else if (cmd == "SUBMIT") {
            string prob, by, team_name, with, status, at;
            int time;
            iss >> prob >> by >> team_name >> with >> status >> at >> time;

            int ti = team_idx[team_name];
            int pi = pidx(prob);
            Team& team = teams[ti];
            ProblemState& ps = team.probs[pi];

            team.subs.push_back({prob, status, time});

            if (frozen_state) {
                ps.frozen_attempts++;
                if (is_ac(status)) {
                    ps.frozen_has_ac = true;
                    if (ps.frozen_ac_time == -1) ps.frozen_ac_time = time;
                }
                if (!ps.solved_before_freeze) ps.is_frozen = true;
            } else {
                if (ps.solved_time == -1) {
                    if (is_ac(status)) {
                        ps.solved_time = time;
                        ps.wrong_before_ac = ps.wrong_attempts;
                        team.solved++;
                        team.penalty += 20 * ps.wrong_attempts + time;
                        team.solve_times.push_back(time);
                        ranking_valid = false;
                    } else {
                        ps.wrong_attempts++;
                    }
                }
            }
        }
        else if (cmd == "FLUSH") {
            flush_ranking();
            cout << "[Info]Flush scoreboard.\n";
        }
        else if (cmd == "FREEZE") {
            if (frozen_state) {
                cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
            } else {
                frozen_state = true;
                do_freeze();
                cout << "[Info]Freeze scoreboard.\n";
            }
        }
        else if (cmd == "SCROLL") {
            if (!frozen_state) {
                cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            } else {
                do_scroll();
                frozen_state = false;
                ranking_valid = false;
            }
        }
        else if (cmd == "QUERY_RANKING") {
            string name;
            iss >> name;
            auto it = team_idx.find(name);
            if (it == team_idx.end()) {
                cout << "[Error]Query ranking failed: cannot find the team.\n";
            } else {
                cout << "[Info]Complete query ranking.\n";
                if (frozen_state)
                    cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
                int r = get_rank(name);
                cout << name << " NOW AT RANKING " << r << "\n";
            }
        }
        else if (cmd == "QUERY_SUBMISSION") {
            string team_name, where, problem_eq, and_str, status_eq;
            iss >> team_name >> where >> problem_eq >> and_str >> status_eq;
            size_t eq1 = problem_eq.find('=');
            string problem_val = (eq1 != string::npos) ? problem_eq.substr(eq1 + 1) : "";
            size_t eq2 = status_eq.find('=');
            string status_val = (eq2 != string::npos) ? status_eq.substr(eq2 + 1) : "";

            auto it = team_idx.find(team_name);
            if (it == team_idx.end()) {
                cout << "[Error]Query submission failed: cannot find the team.\n";
            } else {
                cout << "[Info]Complete query submission.\n";
                int ti = it->second;
                Team& team = teams[ti];
                int found = -1;
                for (int i = (int)team.subs.size() - 1; i >= 0; i--) {
                    const SubRecord& sr = team.subs[i];
                    bool prob_ok = (problem_val == "ALL" || sr.problem == problem_val);
                    bool stat_ok = (status_val == "ALL" || sr.status == status_val);
                    if (prob_ok && stat_ok) {
                        found = i;
                        break;
                    }
                }
                if (found == -1) {
                    cout << "Cannot find any submission.\n";
                } else {
                    const SubRecord& sr = team.subs[found];
                    cout << team_name << " " << sr.problem << " " << sr.status << " " << sr.time << "\n";
                }
            }
        }
        else if (cmd == "END") {
            cout << "[Info]Competition ends.\n";
            break;
        }
    }

    return 0;
}
