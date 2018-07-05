import six

import chainer


def _reduce(grad_list):
    if not grad_list:
        return None
    if len(grad_list) >= 2:
        grad_list[:] = [chainer.functions.add(*grad_list)]
    return grad_list[0]


def _pure(grad):
    return [] if grad is None else [grad]


def _pop_or_none(grad_list):
    return grad_list.pop() if grad_list else None


class GradTable(object):

    """Dict of nodes to references of gradients

    The gradients are stored as references to them in the backprop process. The
    current implementation uses lists. Keep the lengths of lists <= 1 for the
    strict accumulation of gradients. Leave them to accumulate gradients
    lazily.

    Args:
        load_if_new (bool): read ``grad_var`` of node when the node has not
            been added.

    """

    def __init__(self, load_if_new=False):
        self.grads = {}
        self._load_if_new = load_if_new

    def __setitem__(self, node, grad):
        assert node is not None
        self.grads[node] = _pure(grad)

    def get_as_list(self, node):
        assert node is not None
        grads = self.grads
        if node not in grads:
            if self._load_if_new and node.creator_node is None:
                node._check_old_style_gradient()
                # accumulate the gradient only if the node is a leaf
                grads[node] = _pure(node.grad_var)
            else:
                grads[node] = []
        return grads[node]

    def pop(self, node):
        if node is None:
            return None
        grads = self.grads
        if node in grads:
            return _reduce(grads.pop(node))
        if self._load_if_new:
            return node.grad_var
        else:
            return None

    def assert_no_grads(self):
        for gx in self.grads.values():
            assert gx == []


def backprop_step(
        func, target_input_indexes, grad_outputs, grad_inputs):
    assert isinstance(target_input_indexes, tuple)
    assert target_input_indexes == tuple(sorted(target_input_indexes))
    assert isinstance(grad_outputs, tuple)
    if func.backward_accumulate.__code__ \
            is not chainer.FunctionNode.backward_accumulate.__code__:
        # backward_accumulate is overridden

        # Note (Tokui): when the same variable is passed multiple times as
        # inputs in the same function (e.g. an expression like f(x, x)), the
        # current implementation passes None as the current gradient w.r.t.
        # such an input except for the first one (i.e., it builds gxs like
        # (gx, None) where gx is the current gradient w.r.t. x). See also the
        # docstring of ``FunctionNode.backward_accumulate``.
        grad_inputs_tuple = tuple([
            _pop_or_none(grad_inputs[func.inputs[i]])
            for i in target_input_indexes
        ])
        gxs = func.backward_accumulate(
            target_input_indexes, grad_outputs, grad_inputs_tuple)
        assert isinstance(gxs, tuple)
    else:  # otherwise, backward should be overridden
        gxs = func.backward(
            target_input_indexes, grad_outputs)
        # assert isinstance(gxs, tuple)  # this check is failing

        len_gxs = len(gxs)
        if len_gxs == len(func.inputs):
            gxs = tuple([gxs[i] for i in target_input_indexes])
        elif len_gxs != len(target_input_indexes):
            raise ValueError(
                'number of gradients returned by %s (%s) is incorrect.'
                % (func._impl_name, func.label))

    for i, gx in six.moves.zip(target_input_indexes, gxs):
        if gx is not None:
            grad_inputs[func.inputs[i]].append(gx)

    if not func.lazy_grad_sum:
        for gx in grad_inputs.values():
            _reduce(gx)
