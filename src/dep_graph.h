#include "sc.h"

// For each vertex, we need to store an element, its visited flag, its list of edges and a link to the next vertex
typedef struct vertexTag {
    struct ent * ent;
    int visited;
    struct edgeTag * edges;
    struct edgeTag * back_edges;
    struct vertexTag * next;
} vertexT;

/* For each edge, we need a link to the vertex it connects to and a link to the next edge w.r.t source vertex */
typedef struct edgeTag {
   vertexT * connectsTo;
   struct edgeTag * next;
} edgeT;

typedef struct graphCDT {
  vertexT * vertices;
} graphCDT;

typedef struct graphCDT * graphADT;


graphADT GraphCreate();
vertexT * GraphAddVertex(graphADT graph , struct ent * ent);
vertexT * getVertex(graphADT graph, struct ent * ent, int create);
void GraphAddEdge(vertexT * from, vertexT * to);
void print_vertexs();

void destroy_list_edges(edgeT * e);
void destroy_graph (graphADT graph);
void destroy_vertex(struct ent * ent);
void delete_reference(vertexT * v_cur, vertexT * vc, int back_reference);

void markAllVerticesNotVisited();
void ents_that_depends_on (struct ent * ent);
void ents_that_depends_on_range (int r1, int c1, int r2, int c2);
int GraphIsReachable(vertexT * src, vertexT * dest, int back_dep);
void rebuild_graph();

void EvalAll();
void EvalAllVertexs();
void EvalJustOneVertex(register struct ent * p, int i, int j, int rebuild_graph);
