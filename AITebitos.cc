#include "Player.hh"
#include <unordered_map>
#include <unordered_set>
#include <list>

/**
 * Write the name of your player and save this file
 * with the same name and .cc extension.
 */
#define PLAYER_NAME Tebitos

struct PLAYER_NAME : public Player {

  /**
   * Factory: returns a new instance of this class.
   * Do not modify this function.
   */
  static Player* factory () {
    return new PLAYER_NAME;
  }


  /**
   * Types and attributes for your player can be defined here.
   */
  enum tasca {algu_aprop, menjar, matar,matar_z, sense_tasca, no_ini, no_move};

  struct task
  {
    int id;
    Pos pos;
    tasca tasca_act;
    Dir dir;
    int dist;
    int nice;
    Pos pos_ini;
  };

  struct node_q {
    Pos pos_ini;
    Pos pos;
    Dir dir;
    double dist;
    double cost;
    int prio;

    node_q (Pos pos_ini, Pos pos, Dir dir, double dist, double cost, int prio) {
      this->pos_ini = pos_ini;
      this->pos = pos;
      this->dir = dir;
      this->dist = dist;
      this->cost = cost;
      this->prio = prio;
    }
    node_q(){}
    
    bool operator< (const node_q& a) const
    {
      return prio < a.prio;
    }

    bool operator> (const node_q& a) const
    {
      return prio > a.prio;
    }
  };

  struct cmp_q {
    bool operator()(node_q const& t1, node_q const& t2)
    {
      if (t1.prio != t2.prio)
        return t1.prio < t2.prio;
      return t1.pos < t2.pos;
    }
  };

  static bool cmp_l (node_q const& t1, node_q const& t2)
  {
    if (t1.prio != t2.prio)
      return t1.prio < t2.prio;
    return t1.pos < t2.pos;
  }


  vector<Dir> dirs = {Up, Down, Left, Right, DR, RU, UL, LD};
  int mapa[60][60];
  bool weak[4];
  double worth_fighting;
  int dijkstra_max;
  vector<task>tasks;
  queue<Pos>rempl;
  priority_queue<node_q> poss_tasques;

  void initial()
  {
    worth_fighting = 0.4;
    dijkstra_max = 20;
    for (int i = 0; i < 4; ++i) weak[i] = false;
  }

  void ini_vector_tasks(vector<int>& alive)
  {
    task aux;
    aux.tasca_act = no_ini;
    aux.nice = 10;
    aux.dir = RU;
    aux.dist = -1;  
    tasks = vector<task>(0);
    for (int i = 0; i < (int)alive.size(); ++i)
    {
        aux.pos = unit(alive[i]).pos;
        aux.id = alive[i];
        tasks.push_back(aux);
    }
  }

  int pti(Pos p)
  {
    return (p.i + 60*p.j);
  }

  Dir dir_inv(Dir d)
  {
    if (d == Up) return Down;
    if (d == Down) return Up;
    if (d == Left) return Right;
    else return Left;
  }

  bool pos_correct(Pos pos)
  {
    if (pos.i >= 0 and pos.j >= 0 and pos.i < 60 and pos.j < 60 and cell(pos).type == Street) return true;
    return false;
  }

  static bool cmp(task& t1, task& t2)
  {
    return (t1.nice > t2.nice);
  }

  double fight(int pl)
  {
    double f_me = strength(me());
    double u_me = alive_units(me()).size();
    double f_pl = strength(pl);
    double u_pl = alive_units(pl).size();
    if (u_pl == 0 or u_me == 0) return (0.5);
    f_me /= u_me;
    f_pl /= u_pl;
    if (f_me == 0 and f_pl == 0) return (0.5);
    else return (f_me/(f_me + f_pl));
  }

  bool enemy_in_cell(Pos p)
  {
    if (cell(p).id == -1) return (false);
    if (unit(cell(p).id).type == Alive && unit(cell(p).id).player != me()) return true;
    return false;
  }

  bool dead_in_cell(Pos p)
  {
    if (cell(p).id == -1) return (false);
    if (unit(cell(p).id).type == Dead ) return true;
    return false;
  }

  bool zombie_in_cell(Pos p)
  {
    if (cell(p).id == -1) return (false);
    if (unit(cell(p).id).type == Zombie) return true;
    return false;
  }

  bool zombie_near(Pos p)
  {
    //if (pos_correct(p) and cell(p).id != -1 and unit(cell(p).id).type == Zombie) return true;
    for (int i = 0; i < 8; ++i)
    {
      if (pos_correct(p + dirs[i]) and cell(p + dirs[i]).id != -1 and unit(cell(p+ dirs[i]).id).type == Zombie) return true;
    }
    return false;
  }

  bool dead_near(Pos p)
  {
    for (int i = 0; i < 4; ++i)
    {
      if (pos_correct(p + dirs[i]) and cell(p + dirs[i]).id != -1 and unit(cell(p+ dirs[i]).id).type == Dead) return true;
    }
    return false;
  }

  bool enemy_waiting(Pos p)
  {
    if (dead_near(p)) return true;
    return false;
  }

  bool zombie_near(Pos p, Dir d)
  {
    //if (pos_correct(p) and cell(p).id != -1 and unit(cell(p).id).type == Zombie) return true;
    for (int i = 0; i < 8; ++i)
    {
      if (dirs[i] != d and pos_correct(p + dirs[i]) and cell(p + dirs[i]).id != -1 and unit(cell(p+ dirs[i]).id).type == Zombie) return true;
    }
    return false;
  }


  bool strong_enemy_near(Pos p)
  {
    for (int i = 0; i < 4; ++i)
    {
      if (pos_correct(p + dirs[i]) and cell(p + dirs[i]).id != -1 and unit(cell(p+ dirs[i]).id).type == Alive and cell(p+ dirs[i]).owner != me() and weak[cell(p+ dirs[i]).owner] == false) return true;
    }
    return false;
  }

  bool enemy_near(Pos p)
  {
    for (int i = 0; i < 4; ++i)
    {
      if (pos_correct(p + dirs[i]) and cell(p + dirs[i]).id != -1 and unit(cell(p+ dirs[i]).id).type == Alive and cell(p+ dirs[i]).owner != me()) return true;
    }
    return false;
  }

  bool weak_enemy_near(Pos p)
  {
    for (int i = 0; i < 4; ++i)
    {
      if (pos_correct(p + dirs[i]) and cell(p + dirs[i]).id != -1 and unit(cell(p+ dirs[i]).id).type == Alive and cell(p+ dirs[i]).owner != me()) 
      {
        if (alive_units(unit(cell(p + dirs[i]).id).player).size() < tasks .size()) return true;
      }
    }
    return false;
  }

  bool ally_near(Pos p, Dir d)
  {
    for (int i = 0; i < 4; ++i)
    {
      if (dirs[i] != d and pos_correct(p + dirs[i]) and cell(p + dirs[i]).id != -1 and unit(cell(p+ dirs[i]).id).type == Alive and cell(p+ dirs[i]).owner == me()) return true;
    }
    return false;
  }

  void search_weak_players()
  {
    for (int i = 0; i < 4; ++i)
    {
      if (i != me() and fight(i) > worth_fighting)
      {
        weak[i] = true;
      }
    }
  }

  void clean_map()
  {
    for (int i = 0; i < 60; ++i)
    {
      for (int j = 0; j < 60; ++j)
        mapa[i][j] = 0;
    }
  }

  void actualitzar_voltant(Pos p, int n, int r)
  {
    for (int i = -r; i <= r; ++i)
    {
      for (int j = -r; j <=r; ++j)
      {
        if (pos_correct(p + Pos(i, j))) mapa[p.i + i][p.j + j] +=n;
      }
    }
    if (pos_correct(p + Pos(r+1, 0))) mapa[p.i +r+1][p.j] +=n;
    if (pos_correct(p + Pos(-r-1, 0))) mapa[p.i -r-1][p.j] +=n;
    if (pos_correct(p + Pos(0, r+1))) mapa[p.i][p.j + r+1] +=n;
    if (pos_correct(p + Pos(0, -r-1))) mapa[p.i][p.j - r-1] +=n;
  }

  void BFS_voltant(Pos p, int n)
  {
    unordered_set<int> voltant;
    queue<pair<Pos, int> > q;
    q.push(make_pair(p, n));
    voltant.insert(pti(p));
    while (!q.empty() and q.front().second != 0)
    {
      pair<Pos, int> front = q.front();
      q.pop();
      mapa[front.first.i][front.first.j] += front.second;
      for (int i = 0; i < 4; ++i)
      {
        if (pos_correct(front.first + dirs[i]) and voltant.insert(pti(front.first+dirs[i])).second)
        {
          if (front.second > 0)
            q.push(make_pair(front.first+dirs[i], front.second -2));
          else
            q.push(make_pair(front.first+dirs[i], front.second +2));
        }
      }
    }

  }

  void prepare_tasks()
  {
    clean_map(); //aixo es pot estalviar declarant a 0 cada cop i fent despres la suma dels voltants;

    poss_tasques = priority_queue<node_q>();
    //priority_queue<node_q> poss_tasques;
    rempl = queue<Pos>();
    for (int i = 0; i < 60; ++i)
    {
      for (int j = 0; j < 60; ++j)
      {
          if (pos_correct(Pos(i, j)))
          {
            Pos pos(i, j);
            if (cell(pos).food)
            {
              BFS_food(pos, 30);
              BFS_voltant(pos, 8);
            }
            else if (cell(pos).id != -1 and unit(cell(pos).id).type == Zombie)
            {
              BFS_zombie(pos, 30);
              BFS_voltant(pos, 14);
            }
            else if (cell(pos).id != -1 and unit(cell(pos).id).type == Alive and weak[(unit(cell(Pos(i, j)).id).player)] == true)
            {
              BFS_enemy(pos, 15);
              BFS_voltant(pos, 6);
            }
            else if (cell(pos).id != -1 and unit(cell(pos).id).type == Dead)
            {
              BFS_enemy(pos, 20);
              BFS_voltant(pos, 10);
            }
            /*else if (cell(pos).id != -1 and weak[unit(cell(Pos(i, j)).id).player] == false)
            {
              BFS_voltant(pos, -6);
            }*/
            else if (cell(pos).owner != -1 and cell(pos).owner != me())
            {
              mapa[i][j] += 3;
            }
            else if (cell(pos).owner != -1 and cell(pos).owner == me())
            {
              mapa[i][j] += -4;
            }
            else if (cell(pos).owner != -1)
            {
              mapa[i][j] += 2;
            }
          }
      }
      
    }
  }

  void assignar_tasques()
  {
    unordered_set<int> poss_assig = unordered_set<int>();
    unordered_set<int> assignades = unordered_set<int>();

    for (int i = 0; i < (int)tasks.size(); ++i)
    {
      if (tasks[i].tasca_act != no_ini)
      {
        assignades.insert(pti(tasks[i].pos));
      }
    }

    while (!poss_tasques.empty())
    {
      auto front = poss_tasques.top();
      poss_tasques.pop();
      //cerr << front.prio << " " << front.dir << " " << front.pos << front.pos_ini<< endl;
      if (assignades.find(pti(front.pos)) == assignades.end() and poss_assig.find(pti(front.pos_ini)) == poss_assig.end())
      {
        assignades.insert(pti(front.pos));
        if (unit(cell(front.pos).id).rounds_for_zombie == -1) poss_assig.insert(pti(front.pos_ini));
        int num_task = -1;
        int id = cell(front.pos).id;
        for (int i = 0; i < (int)tasks.size() and num_task == -1; ++i) 
        {
          if (tasks[i].id == id)
            num_task = i;
        }

        if (cell(front.pos_ini).food)
        {
          tasks[num_task].tasca_act = menjar;
          tasks[num_task].dir = dir_inv(front.dir);
          tasks[num_task].dist = front.dist;
          if (!enemy_near(front.pos + dir_inv(front.dir))) tasks[num_task].nice = 10;
          else if (enemy_near(front.pos + dir_inv(front.dir)) and cell(front.pos + dir_inv(front.dir)).food) tasks[num_task].nice = 0;
          else tasks[num_task].nice = 2;
        }

        else if (unit(cell(front.pos_ini).id).type == Zombie)
        {
          tasks[num_task].tasca_act = matar_z;
          if (front.dist == 2)
          {
            //cerr << "EIEIEI" << endl;
            tasks[num_task].dir = dirs[4];
          }
          else 
            tasks[num_task].dir = dir_inv(front.dir);
          tasks[num_task].dist = front.dist;
          if (!enemy_near(front.pos + dir_inv(front.dir))) tasks[num_task].nice = 10;
          else tasks[num_task].nice = 1;
        }

        else
        {
          tasks[num_task].tasca_act = matar;
          tasks[num_task].dir = dir_inv(front.dir);
          if (front.dist == 3)
          {
            if (unit(cell(front.pos_ini).id).type == Alive and !enemy_waiting(front.pos_ini) and alive_units(cell(front.pos_ini).owner).size() < tasks.size() and (!zombie_near(front.pos) or unit(tasks[num_task].id).rounds_for_zombie != -1))
            {
              tasks[num_task].dir = RU;
            }
          }
          else if (front.dist == 2)
          {
            if (unit(cell(front.pos_ini).id).type == Alive and !enemy_waiting(front.pos_ini) and alive_units(cell(front.pos_ini).owner).size() > tasks.size() and (!zombie_near(front.pos) or unit(tasks[num_task].id).rounds_for_zombie != -1))
            {
              tasks[num_task].dir = RU;
            }
          }
          tasks[num_task].dist = front.dist;
          if (!enemy_near(front.pos + dir_inv(front.dir))) tasks[num_task].nice = 10;
          else tasks[num_task].nice = 0;
        }
      }
    }
  }

  int compute_prio(const node_q& front)
  {
    if (unit(cell(front.pos).id).rounds_for_zombie != -1) return 100- front.dist;
    if (cell(front.pos_ini).food)
      return (104 - front.dist);
    else if (unit(cell(front.pos_ini).id).type == Zombie)
      return (100 - front.dist);
    else if (unit(cell(front.pos_ini).id).type == Alive)
      return (92 - 3*front.dist + 15*fight(unit(cell(front.pos_ini).id).player));
    else if (unit(cell(front.pos_ini).id).type == Dead)
      return (105 - 4*front.dist);
    return 90-front.dist;
  }

  void BFS_food(Pos pos_ini, int dist_max)
  {
    queue<node_q> q;
    node_q node_ini;
    node_ini.dist = 1;
    node_ini.pos_ini = pos_ini;
    for (int d = 0; d < 4; ++d)
    {
      node_ini.pos = pos_ini + dirs[d];
      node_ini.dir = dirs[d];
      if (pos_correct(node_ini.pos)) q.push(node_ini);
    }
    int trobat = 0;
    node_q front;
    node_ini.pos = pos_ini;
    unordered_map<int, node_q> visitats;
    visitats.insert(make_pair(pti(pos_ini), node_ini));
    node_q near_unit;
    near_unit.dist = -1;
    near_unit.dir = Up;
    node_q near_enemy;
    near_enemy.dist = -1;
    int dist_enemic = 100;
    while (!q.empty() and trobat <3 and q.front().dist < dist_max and (q.front().dist < 2*dist_enemic))
    {
      front = q.front();
      q.pop();
      
      if (cell(front.pos).id != -1 and unit(cell(front.pos).id).player != me()) {near_enemy = front;}
      if ((visitats.insert(make_pair(pti(front.pos), front)).second))
      {
        if (cell(front.pos).id != -1 and (unit(cell(front.pos).id).rounds_for_zombie == -1 or front.dist < unit(cell(front.pos).id).rounds_for_zombie) and unit(cell(front.pos).id).player == me() and unit(cell(front.pos).id).type == Alive and (unit(cell(front.pos).id).rounds_for_zombie != -1 or !zombie_near(front.pos + dir_inv(front.dir))) and !dead_in_cell(front.pos + dir_inv(front.dir))) {front.prio = compute_prio(front); poss_tasques.push(front); trobat++;}
        if (cell(front.pos).id != -1 and unit(cell(front.pos).id).player != me())
        {
          if (dist_enemic == 100) dist_enemic = front.dist;
        }
        int dist = front.dist;
        dist++;
        for (int d = 0; d < 4; ++d)
        {
          if (pos_correct(front.pos + dirs[d])) 
          {
            node_q aux;
            aux.dir = dirs[d];
            aux.pos_ini = front.pos_ini;
            aux.pos = front.pos + dirs[d];
            aux.dist = dist;
            q.push(aux);
          }
        }
      }
    }
  }
  void BFS_zombie(Pos pos_ini, int dist_max)
  {
    queue<node_q> q;
    node_q node_ini;
    node_ini.dist = 1;
    node_ini.pos_ini = pos_ini;
    for (int d = 0; d < 4; ++d)
    {
      node_ini.pos = pos_ini + dirs[d];
      node_ini.dir = dirs[d];
      if (pos_correct(node_ini.pos)) q.push(node_ini);
    }
    int trobat = 0;
    node_q front;
    node_ini.pos = pos_ini;
    unordered_map<int, node_q> visitats;
    visitats.insert(make_pair(pti(pos_ini), node_ini));
    node_q near_unit;
    near_unit.dist = -1;
    near_unit.dir = Up;
    int dist_enemic = 100;
    while (!q.empty() and trobat < 3 and q.front().dist < dist_max and (q.front().dist < 2*dist_enemic))
    { 
      front = q.front();
      q.pop();
      if ((visitats.insert(make_pair(pti(front.pos), front)).second))
      {
        if (cell(front.pos).id != -1 and (unit(cell(front.pos).id).rounds_for_zombie == -1 or front.dist < 2*unit(cell(front.pos).id).rounds_for_zombie)  and unit(cell(front.pos).id).player == me() and unit(cell(front.pos).id).type ==Alive and !dead_in_cell(front.pos + dir_inv(front.dir)) and (!zombie_near(front.pos + dir_inv(front.dir), dir_inv(front.dir)))) {front.prio = compute_prio(front); poss_tasques.push(front); trobat++;}
        if (cell(front.pos).id != -1 and unit(cell(front.pos).id).player != me())
        {
          if (dist_enemic == 100) dist_enemic = front.dist;
        }
        int dist = front.dist;
        dist++;
        for (int d = 0; d < 4; ++d)
        {
          if (pos_correct(front.pos + dirs[d])) 
          {
            node_q aux;
            aux.dir = dirs[d];
            aux.pos_ini = front.pos_ini;
            aux.pos = front.pos + dirs[d];
            aux.dist = dist;
            q.push(aux);
          }
        }
      }
    }
  }

  void BFS_enemy(Pos pos_ini, int dist_max)
  {
    queue<node_q> q;
    node_q node_ini;
    node_ini.dist = 1;
    node_ini.pos_ini = pos_ini;
    for (int d = 0; d < 4; ++d)
    {
      node_ini.pos = pos_ini + dirs[d];
      node_ini.dir = dirs[d];
      if (pos_correct(node_ini.pos)) q.push(node_ini);
    }
    int trobat = 0;
    node_ini.pos = pos_ini;
    node_q front;
    unordered_map<int, node_q> visitats;
    visitats.insert(make_pair(pti(pos_ini), node_ini));
    node_q near_unit;
    near_unit.dist = -1;
    near_unit.dir = Up;

    while (!q.empty() and trobat <3 and q.front().dist < dist_max)
    {
      front = q.front();
      q.pop();
      
      if ((visitats.insert(make_pair(pti(front.pos), front)).second))
      {
        if (cell(front.pos).id != -1 and (unit(cell(front.pos).id).rounds_for_zombie == -1 or front.dist < unit(cell(front.pos).id).rounds_for_zombie)  and  unit(cell(front.pos).id).player == me() and unit(cell(front.pos).id).type == Alive and (unit(cell(front.pos).id).rounds_for_zombie != -1 or !zombie_near(front.pos +dir_inv(front.dir))) and !dead_in_cell(front.pos + dir_inv(front.dir))) {front.prio = compute_prio(front); poss_tasques.push(front); trobat++;}
        int dist = front.dist;
        dist++;
        for (int d = 0; d < 4; ++d)
        {
          if (pos_correct(front.pos + dirs[d])) 
          {
            node_q aux;
            aux.dir = dirs[d];
            aux.pos_ini = front.pos_ini;
            aux.pos = front.pos + dirs[d];
            aux.dist = dist;
            q.push(aux);
          }
        }
      }
    }
  }

  void dijkstra(Pos pos_ini, int u, int max)
  {
    //cerr << "minim " <<pos_ini;
    mapa[pos_ini.i][pos_ini.j] = -50;
    queue<node_q> q;
    node_q node_ini;
    node_ini.dist = 1;
    
    for (int d = 0; d < 4; ++d)
    {
      node_ini.pos = pos_ini + dirs[d];
      node_ini.dir = dirs[d];
      node_ini.cost = 3*mapa[node_ini.pos.i][node_ini.pos.j];
      if (pos_correct(node_ini.pos) and (!zombie_near(node_ini.pos) or unit(cell(pos_ini).id).rounds_for_zombie != -1) and cell(node_ini.pos).id == -1 and !strong_enemy_near(node_ini.pos)) q.push(node_ini);    
    }
    //cerr << q.size() << endl;
    node_q front;
    node_ini.pos = pos_ini;
    unordered_map<int, node_q> visitats;
    visitats.insert(make_pair(pti(pos_ini), node_ini));
    node_q minim;
    minim.dist = -1;
    minim.cost = -1000;
    minim.dir = Up;

    while (!q.empty() and q.front().dist <= max)
    {
      front = q.front();
      //cerr << front.pos << endl;
      q.pop();
      if (front.cost > minim.cost) {minim = front;}
      auto search = visitats.insert(make_pair(pti(front.pos), front));
      if (search.second == true)
      {
        int dist = front.dist;
        dist++;
        for (int d = 0; d < 4; ++d)
        {
          if (pos_correct(front.pos + dirs[d]) and cell(front.pos + dirs[d]).id ==-1) 
          {
            node_q aux;
            aux.dir = front.dir;
            aux.pos = front.pos + dirs[d];
            aux.dist = dist;
            aux.cost = front.cost + mapa[aux.pos.i][aux.pos.j];
            q.push(aux);
            (*(search.first)).second = front;
            //if (cell(aux.pos).owner != me()) mapa[aux.pos.i][aux.pos.j] = 0;
          }
        }
      }
    }
    if (minim.dist != -1 and minim.dist < 30)
    {
      int num_task = -1;
      for (int i = 0; i < (int)tasks.size() and num_task == -1; ++i) 
      {
        if (tasks[i].id == u)
        num_task = i;
      }
      tasks[num_task].tasca_act = sense_tasca;
      tasks[num_task].dir = minim.dir;
      tasks[num_task].dist = minim.dist;
      if (!enemy_near(pos_ini+ minim.dir)) tasks[num_task].nice = 10;
      else tasks[num_task].nice = 3;
    }
  }

  void fight_near_enemy(int id)
  {
    Pos pos = unit(id).pos;
    int num_task = -1;
    for (int i = 0; i < (int)tasks.size() and num_task == -1; ++i) 
    {
      if (tasks[i].id == id)
      num_task = i;
    }
    for (int i = 0; i < 4; ++i)
    {
      if (pos_correct(pos + dirs[i]) and enemy_in_cell(pos + dirs[i]) and (!zombie_near(pos) or unit(id).rounds_for_zombie != -1)/*and fight(unit(cell(pos + dirs[i]).id).player) > 0.5*/)
      {
        //if (weak[unit(cell(pos + dirs[i]).id).player])
        //{
          
          //if (tasks[num_task].tasca_act != algu_aprop)
          {
            tasks[num_task].tasca_act = algu_aprop;
            tasks[num_task].dir = dirs[i];
            tasks[num_task].dist = 1;
            tasks[num_task].nice = 25 + (int)(10*fight(unit(cell(pos + dirs[i]).id).player));
            if (dead_near(pos) or zombie_near(pos)) tasks[num_task].nice += 10;
          }
        /*}
        else
        {
          if (i != 0 and pos_correct(pos + dirs[0])) { move(id, dirs[0]); return (true);}
          else if (i != 1 and pos_correct(pos + dirs[1])) { move(id, dirs[1]); return (true);}
          else if (i != 2 and pos_correct(pos + dirs[2])) { move(id, dirs[2]); return (true);}
          else if (i != 3 and pos_correct(pos + dirs[3])) { move(id, dirs[3]); return (true);}
          else {move(id, dirs[i]); return (true);}
        }*/
      }
    }
    for (int i = 0; i < 4; ++i)
    {
      if (pos_correct(pos + dirs[i]) and zombie_in_cell(pos + dirs[i]))
      {
        if (!enemy_near(pos) and (!zombie_near(pos, dirs[i]) or unit(id).rounds_for_zombie != -1))
        {
          if (tasks[num_task].tasca_act == no_ini)
          {
          tasks[num_task].tasca_act = algu_aprop;
          tasks[num_task].dir = dirs[i];
          tasks[num_task].dist = 1;
          if (enemy_near(pos + dirs[i])) tasks[num_task].nice = 40;
          if (enemy_near(pos)) tasks[num_task].nice = 20;
          }
        }
      }
    }
    for (int i = 0; i < 4; ++i)
    {
      if (pos_correct(pos + dirs[i]) and dead_in_cell(pos + dirs[i]) and (!zombie_near(pos) or unit(id).rounds_for_zombie != -1) and (unit(id).rounds_for_zombie == -1 or unit(id).rounds_for_zombie > unit(cell(pos + dirs[i]).id).rounds_for_zombie))
      {
        int num_task = -1;
        for (int i = 0; i < (int)tasks.size() and num_task == -1; ++i) 
        {
          if (tasks[i].id == id)
          num_task = i;
        }
        for (int j = 0; j < 4; ++j)
        {
          if (dirs[j] != dirs[i] and weak_enemy_near(pos + dirs[j]))
          {
            if (tasks[num_task].tasca_act == no_ini)
            {
              tasks[num_task].tasca_act = matar;
              tasks[num_task].dir = dirs[j];
              tasks[num_task].dist = 1;
              tasks[num_task].nice = -1;
            } 
          }
        }
        if (tasks[num_task].tasca_act == no_ini)
        {
          tasks[num_task].tasca_act = no_move;
          tasks[num_task].dir = dirs[i];
          tasks[num_task].dist = 1;
          if (!enemy_near(pos + dirs[i])) tasks[num_task].nice = 10;
          else tasks[num_task].nice = 10;
        }
      }
    }
    for (int i = 0; i < 4; ++i)
    {
      if (pos_correct(pos + dirs[i]) and cell(pos+dirs[i]).food and (!zombie_near(pos + dirs[i]) or unit(id).rounds_for_zombie != -1))
      {
        if (tasks[num_task].tasca_act == no_ini or enemy_near(pos))
        {
          if (!enemy_near(pos)) tasks[num_task].nice = 10;
          else tasks[num_task].nice = 30; 
          tasks[num_task].tasca_act = algu_aprop;
          tasks[num_task].dir = dirs[i];
          tasks[num_task].dist = 1;
          if (tasks[num_task].nice == 30) return;
        }
      }
    }
    
    if (tasks[num_task].tasca_act == no_ini and (enemy_near(unit(id).pos)))
    {
      for (int i = 0; i < 4; ++i)
      {
        if (cell(pos + dirs[i]).id != -1 and unit(cell(pos + dirs[i]).id).type == Alive and fight(cell(pos + dirs[i]).id) < 0.5)
        {
          tasks[num_task].tasca_act = algu_aprop;
          tasks[num_task].dir = dirs[i];
          tasks[num_task].dist = 1;
          tasks[num_task].nice = 30;
          return;
        }
      }
    }
    if (tasks[num_task].tasca_act == no_ini and (zombie_near(unit(id).pos)))
    {
      for (int i = 0; i < 4; ++i)
      {
        if (cell(pos + dirs[i]).id != -1 and unit(cell(pos + dirs[i]).id).type == Zombie)
        {
          tasks[num_task].tasca_act = algu_aprop;
          tasks[num_task].dir = dirs[i];
          tasks[num_task].dist = 1;
          tasks[num_task].nice = 30;
          return;
        }
      }
    }
    if (tasks[num_task].tasca_act == no_ini and (enemy_near(unit(id).pos)))
    {
      for (int i = 0; i < 4; ++i)
      {
        if (cell(pos + dirs[i]).id != -1 and unit(cell(pos + dirs[i]).id).type == Alive and fight(cell(pos + dirs[i]).id) >= 0.5)
        {
          tasks[num_task].tasca_act = algu_aprop;
          tasks[num_task].dir = dirs[i];
          tasks[num_task].dist = 1;
          tasks[num_task].nice = 30;
          return;
        }
      }
    }
  }


  /**
   * Play method, invoked once per each round.
   */
  virtual void play () 
  {
    double st = status(me());
    if (st >= 0.87) 
    {
      for (int u = 0; u < (int)tasks.size(); ++u)
      {
        vector<int> alive = alive_units(me());
        ini_vector_tasks(alive);
        search_weak_players();
        fight_near_enemy(alive[u]);
      }
      for (int u = 0; u < (int)tasks.size(); ++u)
      {
        if (tasks[u].tasca_act != no_ini) move(tasks[u].id, tasks[u].dir);
      }
      return;
    }

    if (round() == 0) initial();
    vector<int> rand = random_permutation(4);
    vector<Dir> aux(0);
    for (int i = 0; i < 4; ++i)
    {
      aux.push_back(dirs[rand[i]]);
    }
    for (int i = 0; i < 4; ++i)
    {
      dirs[i] = aux[i];
    }
    vector<int> alive = alive_units(me());
    ini_vector_tasks(alive);
    search_weak_players();
    for (int u = 0; u < (int)tasks.size(); ++u)
    {
      fight_near_enemy(alive[u]);
    }
    prepare_tasks();
    assignar_tasques();
    for (int u = 0; u < (int)tasks.size(); ++u)
    {
      if (tasks[u].tasca_act == no_ini)
      {
        if (unit(tasks[u].id).rounds_for_zombie != -1)
          dijkstra(tasks[u].pos, tasks[u].id, unit(tasks[u].id).rounds_for_zombie);
        else
          dijkstra(tasks[u].pos, tasks[u].id, 20);
      }
    }
    sort(tasks.begin(), tasks.end(), cmp);

    for (int u = 0; u < (int)tasks.size(); ++u)
    {
      move(tasks[u].id, tasks[u].dir);
      /*if (tasks[u].tasca_act == menjar) cerr << "Menjar";
      if (tasks[u].tasca_act == matar) cerr << "Matar";
      if (tasks[u].tasca_act == matar_z) cerr << "Matar zombie ";
      if (tasks[u].tasca_act == algu_aprop) cerr << "Algu aprop";
      if (tasks[u].tasca_act == sense_tasca) cerr << "Cap";
      if (tasks[u].tasca_act == no_ini) cerr << "no ini";
      if (tasks[u].tasca_act == no_move) cerr << "no move";
      
      cerr <<tasks[u].dir << " " << tasks[u].pos << endl;*/
    }
  }
};


/**
 * Do not modify the following line.
 */
RegisterPlayer(PLAYER_NAME);
