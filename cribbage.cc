#include <iostream>
#include <vector>
#include <cassert>
#include <map>
#include <deque>
#include <limits>
using namespace std;
using ll = int64_t;
using score = int16_t;

// Cards are just represented as integers from 1 to 13
// This translates back and forth between cards and strings
map<string,ll> CARDS = {
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

ll cardFromString(const string& S) {
  assert(CARDS.count(S) == 1);
  return CARDS[S];
}
string cardToString(ll x) {
  for(auto& kv : CARDS) {
    if(kv.second == x) {
      return kv.first;
    }
  }
  assert(false);
}

// 10 J Q K all contribute 10 to the stack total
ll value_of(ll card) {
  return card<=10 ? card : 10LL;
}

// The input deal. 4 stacks of 13 cards, from bottom to top
// (i.e. the first playable card is first in each list).
// This is the *opposite* of the order from the input file, where
// the first playable card is listed last (top to bottom).
vector<vector<ll>> IN;

// A possible position for the game
// One way to represent this would be the list of moves we have taken so far,
// where a "move" is just which pile to take the next card from
// (or clearing the current stack if we can't play from any pile).
// However, that would be too many states - approximately 5*10^28 possibilities.
//
// The key challenge in dynamic programming is to condense the large state space into
// a smaller one by "forgetting" certain information. What do we really need to know to
// be able to score the game going forward?
//
// My answer is:
// 1) We need to know how many cards are left in each pile (I)
// 2) We need to know the current stack total (sum), so that we know when to switch stacks
//    and so that we can give points for hitting 15 and 31.
// 3) We need to know the top 6 cards of the stack to score sets and straights
//    Sets would be easy; we could just remember the top card and how many copies of it we have.
//    But straights are harder. Straights can be at most 7 cards, so if we remember the top 6
//    cards that will be enough to tell if the next card forms a 7-card-straight.
//    The top 6 cards is clearly enough to score sets (the top 3 cards would be enough for that).
//
// How big is this state space? We have between 0 and 13 cards left in each pile - 14 options for each.
// There are 32 possible stack totals (anything from 0 to 31).
// There are 13 possibilities for each of the 6 most recent cards in the stack.
// So the total state space here is 14**4 * 32 * 13**6 = 5*10^12. This is a big improvement on our first
// try, but still too much.
//
// However, there is one more observation. Instead of tracking which cards are at the top of the current
// stack, we can track which piles we took those cards from. Since we know how many cards are left in each
// pile, this tells us which cards they are. For example, if we know the top card of the stack came from pile
// 1, and there are no cards left in pile 1, the top card must have been the last card that was
// initially in pile 1, since moving it emptied the pile. And so on.
//
// This is an improvement, since there are 13 possible cards but only 4 possible moves.
// This takes the state space down to 14**4 * 32 * 4**6 = 5*10^9, which is small enough.
struct State {
  // How many cards we have removed from each pile (length 4)
  vector<ll> I;
  // Total of current stack
  ll sum = 0;
  // The top 6 cards of the current stack (if the current stack has less than 6 cards, the whole thing)
  deque<ll> STACK;
  // Which piles the cards in STACK came from
  deque<ll> MOVES;

  // Initially we haven't taken any cards from any piles and the stack is empty
  State() : I(vector<ll>(4, 0)), sum(0), STACK(deque<ll>{}), MOVES(deque<ll>{}) {}
  State(const State* o) : I(o->I), sum(o->sum), STACK(o->STACK), MOVES(o->MOVES) {}

  // Are we done with the game i.e. is every pile empty?
  bool done() const {
    for(ll i=0; i<IN.size(); i++) {
      if(I[i] < IN[i].size()) { return false; }
    }
    return true;
  }
  // Can we take a card from this pile now i.e. is the pile non-empty
  // and adding it to the current stack wouldn't exceed 31?
  bool valid_move(ll move) const {
    return I[move] < IN[move].size() && (sum + value_of(card_of(move)) <= 31);
  }
  // Which card is currently on the top of pile [move]
  ll card_of(ll move) const {
    return IN[move][I[move]];
  }
  // How many points would we immediately score by playing the card on top of pile [move]
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
    // Sets
    // Count how many cards on top of the STACK match the new card
    if(STACK.size()>=1 && STACK.back() == card) {
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

    // Straights
    // This is tricky. Maybe there's a simpler way to do it?
    // The idea is that a set of cards form a straight if they are
    // all different and the difference between the min and the max card
    // is one less than the number of cards. For example 2 3 1 form a straight
    // because max(2,3,1)-min(2,3,1)=3-1=2=len(3,2,1)-1.
    // Add cards from the back one-by-one, checking if they are all unique and
    // satisfy the min/max condition.
    // Score points equal to the length of the longest straight, as long as
    // its at least length 3.
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

  // What State would we be in if we played the card on top of pile [move]?
  State move(ll move) const {
    assert(valid_move(move));
    State S2(this);
    assert(move<IN.size() && move<I.size() && I[move]<IN[move].size());
    ll card = IN[move][I[move]];
    S2.I[move]++;
    S2.sum += value_of(card);
    S2.STACK.push_back(card);
    // We only want to track the top 6 cards of the stack
    if(S2.STACK.size()>=7) { S2.STACK.pop_front(); }
    S2.MOVES.push_back(move);
    if(S2.MOVES.size()>=7) { S2.MOVES.pop_front(); }
    return S2;
  }
  // What State would we be in if we started a new stack?
  State clear() const {
    State S2(this);
    S2.sum = 0;
    S2.STACK.clear();
    S2.MOVES.clear();
    return S2;
  }

  // To run the DP, we need to write down the answers for each state in a big table.
  // This table is going to be big...there are around 5 billion states so there
  // will be 5 billion entries in the table.
  // To get decent performance, the table should be a big array (not a hashmap).
  // This function tells the State what its index into that array is.
  // It must produce a different index for each state, and we shouldn't waste any indices
  // (i.e. each State should map to exactly one integer between 0 and the number of states).
  //
  // We can think of writing the State as a number in two steps.
  // First, we write out the state as a big tuple:
  // (MOVE[0], MOVE[1], MOVE[2], MOVE[3], MOVE[4], MOVE[5], sum, I[0], I[1], I[2], I[3])
  //
  // (We don't need to include STACK, because STACK can be computed from MOVE and I).
  //
  // Then we convert that tuple into a number by thinking of the elements of the tuple as "digits".
  // MOVE[0] is the "digit" in the "ones" place.
  // MOVE[1] is the "digit" in the 4s place (since there were four options for MOVE[0]).
  // ..
  // sum is the "digit" in the 4^6 place (since there are 4^6 options for MOVE).
  // I[0] is the "digit" in the 4^6*32 place
  //
  // This two-step strategy is usually a good way to convert a DP State into an index.
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
// Write out the State (for debugging)
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
  o << " | sum=" << S.sum << endl;
  return o;
}

// The 5-billion element DP table.
// Because the table is so large, we only store a 16-bit number in each position.
vector<score> DP;
// dp(State) = How many points can we get from the rest of the game if we play perfectly?
// To compute this, just try all possible moves. There are only 4 - we could play a card from
// any of the four piles. If we can't play a card from any pile, start a new stack.
score dp(const State& S) {
  // Once we've played every card, we can't score any more points.
  if(S.done()) { return 0LL; }
  ll key = S.key();
  // Assert the key() function returned a valid index
  assert(0<=key && key<DP.size());
  // If we've already seen this state, return the precomputed answer
  if(DP[key]>=0) { return DP[key]; }

  // Consider playing from each pile.
  ll ans = -1;
  for(ll move=0; move<4; move++) {
    if(S.valid_move(move)) {
      ll points = S.points_of(move);
      State S2 = S.move(move);
      // Our score for playing from this pile are the points
      // we immediately score plus the points we can get from
      // the rest of the game (which is a recursive call).
      ll move_score = points + static_cast<ll>(dp(S2));
      // We want to play the move that maximizes our score
      if(move_score > ans) {
        ans = move_score;
      }
    }
  }

  // If there are no valid moves, just clear the stack
  if(ans == -1) {
    State S2 = S.clear();
    ans = static_cast<ll>(dp(S2));
  }

  // Check that we are using enough bits to write down the score
  assert(ans <= std::numeric_limits<score>::max());
  DP[key] = static_cast<score>(ans);
  return static_cast<score>(ans);
}

int main() {
  // Read in the input
  IN = vector<vector<ll>>(4, vector<ll>(13, 0));
  vector<ll> C(14, 0);
  for(ll i=0; i<4; i++) {
    for(ll j=0; j<13; j++) {
      string S;
      cin >> S;
      ll card = cardFromString(S);
      // Reverse the order of each pile
      IN[i][13-1-j] = card;
      C[card]++;
    }
  }
  // Check that we have 4 cards of each rank. This will catch input mistakes.
  for(ll i=1; i<=13; i++) {
    if(C[i] != 4) {
      cerr << "i=" << i << " count=" << C[i] << endl;
      assert(false);
    }
  }

  ll sz = 14LL*14*14*14 * 32 * 4*4*4*4*4*4;
  DP = vector<score>(sz, -1);

  // Compute the highest possible score
  State S;
  score best = dp(S);
  cout << "Best possible score: " << static_cast<ll>(best) << endl;

  // Figure out the best possible sequence of moves.
  // It is typical in DP that figuring out the optimal score is easier
  // than finding a "path" to that score. Extracting a path basically
  // requires running the DP again.
  // The idea is: you know how many points you can get if you play perfectly
  // (that's what the DP tells you). So try all possible moves, and
  // make the move that allows you to still get the perfect score. That must
  // be the perfect move.
  ll points = 0;
  while(!S.done()) {
    // Check that we can still get a perfect score
    assert(points+dp(S) == best);

    bool found_move = false;
    for(ll move=0; move<4; move++) {
      // If this move is valid and gives us a perfect score, do it (and print it out)
      if(!found_move && S.valid_move(move)) {
        if(points + S.points_of(move)+dp(S.move(move))==best) {
          found_move = true;
          points += S.points_of(move);
          cerr << "Play card from stack " << (move+1) << " (which is a " << cardToString(S.card_of(move)) << ") scoring " << S.points_of(move) << " total=" << points << endl;
          S = S.move(move);
        }
      }
    }
    // The other possible "move" is starting a new stack. If we didn't play a card from a pile,
    // we must have done that.
    if(!found_move) {
      assert(points + dp(S.clear())==best);
      cerr << "NEW STACK" << endl;
      S = S.clear();
    }
  }
}
