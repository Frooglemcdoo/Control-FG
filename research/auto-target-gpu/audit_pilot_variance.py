"""Exact two-region estimator audit; synthetic distributions, not game measurements."""
import itertools
import json
import math
from pathlib import Path


def binomial(n, p):
    return [(k, math.comb(n, k) * p**k * (1-p)**(n-k)) for k in range(n+1)]


def summary(events, expected):
    mass = sum(prob for prob, value in events)
    mean = sum(prob * value for prob, value in events)
    variance = sum(prob * (value-mean)**2 for prob, value in events)
    assert abs(mass-1) < 1e-12
    assert abs(mean-expected) < 1e-11
    return {'mean': mean, 'variance': variance,
            'maximum_possible': max(value for prob, value in events if prob > 0)}


def audit(p, guide, rare=1., other=0.):
    expected = p*rare+(1-p)*other
    old_alpha = .1
    q = (1-old_alpha)*p+old_alpha*guide
    wr, wb = p/q, (1-p)/(1-q)
    old = []
    for pilot, pilot_probability in [(rare, p), (other, 1-p)]:
        for hits, probability in binomial(9, q):
            sum_weight = hits*wr+(9-hits)*wb
            weighted = hits*wr*rare+(9-hits)*wb*other
            old.append((pilot_probability*probability,
                        (1-sum_weight/10)*pilot+weighted/10))
    alpha = .4
    q = (1-alpha)*p+alpha*guide
    wr, wb = p/q, (1-p)/(1-q)
    new = []
    # Four guide and six cosine samples. Within-component strata can reduce
    # this IID component model's variance further; actual GPU tests are separate.
    for (guided, pg), (base, pb) in itertools.product(binomial(4, guide), binomial(6, p)):
        hits = guided+base
        new.append((pg*pb, (hits*wr*rare+(10-hits)*wb*other)/10))
    native = [(prob, (hits*rare+(10-hits)*other)/10) for hits, prob in binomial(10, p)]
    return {'cosine_probability':p, 'guide_probability':guide, 'rare_rgb_scalar':rare,
            'other_rgb_scalar':other, 'native_iid':summary(native, expected),
            'r448_pilot':summary(old, expected), 'r449_fixed_mixture':summary(new, expected)}


if __name__ == '__main__':
    cases = [audit(p, guide, rare, other)
             for p in [.0001, .001, .01, .05]
             for guide in [0, p, 1]
             for rare, other in [(1, 0), (1, 1), (10, .2)]]
    output = Path(__file__).with_name('pilot-variance-audit.json')
    output.write_text(json.dumps({'scope':'Synthetic exact finite enumeration; not Control image/FPS evidence',
                                  'cases':cases}, indent=2)+'\n')
    print(f'PASS {len(cases)} distributions: mean energy preserved for native, R448 and R449.')
