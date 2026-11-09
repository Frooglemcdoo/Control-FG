import unittest
from gi_candidate_contract import Ray,choices,expectation


class CandidateContractTests(unittest.TestCase):
    def test_mixed_hits_misses_and_colored_energy(self):
        rays=[Ray(0,7,(.5,1,2),(2,3,4)),Ray(1,65535,(1,1,1),(6,3,1)),Ray(2,8,(1,1,1),(4,1,9))]
        outcomes=choices((16,16),rays,3,.75)
        for got,want in zip(expectation((16,16),outcomes),(2.75,1.75,4.5)):
            self.assertAlmostEqual(got,want,places=12)
        for outcome in outcomes:self.assertEqual(outcome.fallback,(1.5,.75,.25))
    def test_all_misses_preserved(self):
        rays=[Ray(0,65535,(1,1,1),(2,3,4))]
        self.assertEqual(expectation((0,0),choices((0,0),rays,1,1)),(2,3,4))
    def test_black_hit_still_counts_in_denominator(self):
        rays=[Ray(0,1,(1,1,1),(0,0,0)),Ray(1,2,(1,1,1),(2,2,2))]
        self.assertEqual(expectation((0,0),choices((0,0),rays,2,1)),(1,1,1))
    def test_missing_ray_rejected(self):
        with self.assertRaisesRegex(ValueError,'Missing'):choices((0,0),[],6,1)
    def test_invalid_surface_sentinel_rejected(self):
        with self.assertRaisesRegex(ValueError,'sentinel'):choices((0,0),[Ray(0,65534,(1,1,1),(0,0,0))],1,1)
    def test_nonfinite_rejected(self):
        with self.assertRaisesRegex(ValueError,'Nonfinite'):choices((0,0),[Ray(0,1,(1,1,1),(float('nan'),0,0))],1,1)
    def test_source_contribution_cannot_move_to_neighbor(self):
        outcome=choices((0,0),[Ray(0,1,(1,1,1),(1,1,1))],1,1)[0]
        with self.assertRaisesRegex(ValueError,'Cross-pixel'):outcome.evaluate((1,0))


if __name__=='__main__':unittest.main()
