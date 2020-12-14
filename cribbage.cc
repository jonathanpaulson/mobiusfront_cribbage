#include <iostream>
#include <vector>
#include <cassert>
#include <map>
#include <deque>
#include <limits>
using namespace std;
using ll = int64_t;
using score = int16_t;

ll parse(const string& S){ 
  static map<string,ll> M = {
      {"A", 1},
      {"2", 2},
      {"3", 3},
      {"4", 4},
      {"5", 5},
      {"6", 6},
      {"7", 7},
      {"8", 8},
      {"9", 9},
      {"10", 10},
      {"J", 11},
      {"Q", 12},
      {"K", 13}};
  assert(M.count(S) == 1);
  return M[S];
}
ll value_of(ll card) {
  return card<=10 ? card : 10LL;
}

vector<vector<ll>> IN;
struct State{
  vector<ll> I; // position within each list
  ll sum = 0; // sum of the current stack
  deque<ll> STACK; // the last 6 cards on the stack
  deque<ll> MOVES; // the last 6 moves
  ll depth = 0;

  State() : I(vector<ll>(4, 0)), sum(0), STACK(deque<ll>{}), MOVES(deque<ll>{}), depth(0) {}
  State(vector<ll>& I_, ll sum_, deque<ll>& STACK_, deque<ll>& MOVES_) : I(I_), sum(sum_), STACK(STACK_), MOVES(MOVES_), depth(0) {}
  State(const State* o) : I(o->I), sum(o->sum), STACK(o->STACK), MOVES(o->MOVES), depth(o->depth) {}

  bool done() const {
    for(ll i=0; i<IN.size(); i++) {
      if(I[i] < IN[i].size()) { return false; }
    }
    return true;
  }
  bool valid_move(ll move) const {
    return I[move] < IN[move].size() && (sum + value_of(card_of(move)) <= 31);
  }
  ll card_of(ll move) const {
    return IN[move][I[move]];
  }
  ll points_of(ll move) const {
    assert(valid_move(move));
    ll card = card_of(move);
    ll value = value_of(card);
    ll ans = 0;
    if(card == 11 && sum == 0) { // initial jack
      ans += 2;
    }
    if(sum + value == 15) {
      ans += 2;
    }
    if(sum + value == 31) {
      ans += 2;
    } 
    if(STACK.size()>=1 && STACK.back() == card) { // pair or more
      ll count = 0;
      ll idx = STACK.size()-1;
      while(idx>=0 && STACK[idx]==card) {
        count++;
        idx--;
      }
      assert(count >= 1);
      if(count == 1) { ans += 2; } // pair
      else if(count == 2) { ans += 6; } // triple
      else if(count == 3) { ans += 12; } // quad
    }

    ll straight_size = 0;
    bool ok = true;
    vector<int> SEEN(14, 0);
    ll max_ = card;
    ll min_ = card;
    SEEN[card] = true;
    for(ll sz=2; sz<=7; sz++) {
      ll idx = STACK.size()-sz+1;
      if(idx>=0) {
        ll new_card = STACK[STACK.size()-sz+1];
        if(SEEN[new_card]) {
          ok = false;
        } else {
          SEEN[new_card] = true;
          max_ = max(max_, new_card);
          min_ = min(min_, new_card);
          if(ok && max_ - min_ == sz-1) {
            straight_size = max(straight_size, sz);
          }
        }
      }
    }
    if(straight_size >= 3) {
      ans += straight_size;
    }

    return ans;
  }

  State move(ll move) const {
    assert(valid_move(move));
    State S2(this);
    assert(move<IN.size() && move<I.size() && I[move]<IN[move].size());
    ll card = IN[move][I[move]];
    S2.I[move]++;
    S2.sum += value_of(card);
    S2.STACK.push_back(card);
    if(S2.STACK.size()>=7) { S2.STACK.pop_front(); }
    S2.MOVES.push_back(move);
    if(S2.MOVES.size()>=7) { S2.MOVES.pop_front(); }
    S2.depth++;
    return S2;
  }
  State clear() const {
    State S2(this);
    S2.sum = 0;
    S2.STACK.clear();
    S2.MOVES.clear();
    S2.depth++;
    return S2;
  }

  ll key() const { 
    ll ans = 0;
    ll product = 1;
    assert(MOVES.size()<=6);
    for(ll i=0; i<6; i++) {
      if(i<MOVES.size()) {
        assert(MOVES[i]<4);
        ans += MOVES[i]*product;
      }
      product *= 4;
    }
    assert(sum < 32);
    ans += sum*product;
    product *= 32;
    assert(I.size() == 4);
    for(ll i : I) {
      assert(i<14);
      ans += i*product;
      product *= 14;
    }
    return ans;
  }
};
ostream& operator<<(ostream& o, const State& S) {
  for(ll i=0; i<4; i++) {
    for(ll j=0; j<13; j++) {
      if(j==S.I[i]) { o << "| "; }
      o << IN[i][j] << " ";
    }
    if(S.I[i]==13) { o << "|"; }
    o << endl;
  }
  for(auto& card : S.STACK) {
    o << card << " ";
  }
  o << " | sum=" << S.sum << " depth=" << S.depth << endl;
  return o;
}

vector<score> DP;
score dp(const State& S) {
  //cerr << S << endl;
  if(S.depth >= 100) {
    cerr << S << endl;
    assert(false);
  }
  if(S.done()) { return 0LL; }
  ll key = S.key();
  if(!(0<=key && key<DP.size())) {
    cerr << key << " " << DP.size() << endl;
    assert(false);
  }
  if(DP[key]>=0) { return DP[key]; }

  ll ans = -1;
  ll best_move = -1;
  for(ll move=0; move<4; move++) {
    if(S.valid_move(move)) {
      ll points = S.points_of(move);
      assert(points <= 20);
      State S2 = S.move(move);
      /*
         S2.I[move]++;
         S2.sum = S.sum + value;
         S2.STACK.push_back(card);
         ll old_card = -1;
         if(S2.STACK.size() >= 7) {
         old_card = S2.STACK.front();
         S2.STACK.pop_front();
         }
         ll old_move = -1;
         S2.MOVES.push_back(move);
         if(S2.MOVES.size()>=7) {
         old_move = S2.MOVES.front();
         S2.MOVES.pop_front();
         }*/

      ll move_score = points + static_cast<ll>(dp(S2));
      if(S.depth >= 90) {
        cerr << "===============" << endl << S << "move=" << move << " move_score=" << move_score << " points=" << points << endl;
        assert(false);
      }

      // Restore S
      /*S.I[move]--;
        S.STACK.pop_back();
        if(old_card >= 0) { S.STACK.push_front(old_card); }
        S.MOVES.pop_back();
        if(old_move >= 0) { S.MOVES.push_front(old_move); }*/

      if(move_score > ans) {
        ans = move_score;
        best_move = move;
      }
    }
  }

  if(ans == -1) { // no valid move; clear the stack
    State S2 = S.clear();
    if(S2.depth >= 90) {
      cerr << S << endl << "==========" << endl << S2 << endl;
      assert(false);
    }
    best_move = 10;
    ans = static_cast<ll>(dp(S2));
  }

  (void)best_move;
  /*if(ans >= 300) {
    cerr << "========= FINAL ====== " << endl << S << "ans=" << ans << " best=" << best_move << endl << "======================" << endl;
    assert(false);
    }*/

  assert(ans <= std::numeric_limits<score>::max());
  DP[key] = static_cast<score>(ans);
  return static_cast<score>(ans);
}

int main() {
  IN = vector<vector<ll>>(4, vector<ll>(13, 0));
  vector<ll> C(14, 0);
  for(ll i=0; i<4; i++) {
    for(ll j=0; j<13; j++) {
      string S;
      cin >> S;
      ll card = parse(S);
      IN[i][13-1-j] = card;
      C[card]++;
    }
  }
  for(ll i=1; i<=13; i++) {
    if(C[i] != 4) {
      cerr << "i=" << i << " count=" << C[i] << endl;
      assert(false);
    }
  }

  ll sz = 14LL*14*14*14 * 32 * 4*4*4*4*4*4;
  DP = vector<score>(sz, -1);

  vector<ll> I{0,0,0,0};
  deque<ll> stack;
  deque<ll> moves;
  State S;
  score best = dp(S);
  cout << "Best possible score: " << static_cast<ll>(best) << endl;

  ll points = 0;
  while(!S.done()) {
    assert(points+dp(S) == best);

    bool found_move = false;
    for(ll move=0; move<4; move++) {
      if(!found_move && S.valid_move(move)) {
        if(points + S.points_of(move)+dp(S.move(move))==best) {
          found_move = true;
          points += S.points_of(move);
          cerr << "Play card from stack " << (move+1) << " (which is a " << S.card_of(move) << ") scoring " << S.points_of(move) << " total=" << points << endl;
          S = S.move(move);
        }
      }
    }
    if(!found_move) {
      assert(points + dp(S.clear())==best);
      cerr << "NEW STACK" << endl;
      S = S.clear();
    }
  }
}
