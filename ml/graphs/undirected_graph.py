# -*- coding: utf-8 -*-
import numpy as np
import networkx as nx
import matplotlib.pyplot as plt

from utils.log import getLogger
from utils.constants import BIT_PRED


class UndirectedGraph(object):

  def __init__(self, adjacency_matrix_file):
    self.logger = getLogger('undirected_graph')
    self.graph = nx.from_numpy_matrix(np.loadtxt(adjacency_matrix_file,
                                                 dtype=bool, delimiter=','))
    self.graph = self.postProcess(self.graph)


  def visualizeGraph(self, img_file):
    self.logger.info('Visualizing undirected Bayesian network...')

    plt.close()
    nx.draw_spectral(self.graph, with_labels=False, width=0.1, node_size=5)
    plt.savefig(img_file)


  def postProcess(self, graph):
    components = [graph.subgraph(c) for c in nx.connected_components(graph)]
    relevant_component = [g for g in components if BIT_PRED in g.nodes()][0]

    msg = 'The optimized BN has %d edges.\n' % graph.number_of_edges()
    msg += '\tconnected = {}\n'.format(len(components) == 1)
    msg += '\tnum connected components = %d\n' % len(components)
    largest_cc = max(nx.connected_components(graph), key=len)
    msg += '\tlargest component has %d nodes\n' % len(largest_cc)
    msg += '\tcomponent with bit %d has %d nodes' % (BIT_PRED, relevant_component.number_of_nodes())
    self.logger.info(msg)

    return relevant_component
