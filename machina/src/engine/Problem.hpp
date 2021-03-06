/*
  This file is part of Machina.
  Copyright 2007-2013 David Robillard <http://drobilla.net>

  Machina is free software: you can redistribute it and/or modify it under the
  terms of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or any later version.

  Machina is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Machina.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MACHINA_PROBLEM_HPP
#define MACHINA_PROBLEM_HPP

#include <stdint.h>

#include <map>
#include <stdexcept>

#include "machina/Machine.hpp"
#include "eugene/Problem.hpp"

#include "MIDISink.hpp"

namespace machina {

class Problem : public eugene::Problem<Machine> {
public:
	Problem(TimeUnit           unit,
	        const std::string& target_midi,
	        SPtr<Machine>      seed = SPtr<Machine>());
	virtual ~Problem() {}

	void seed(SPtr<Machine> parent) { _seed = parent; }

	float evaluate(const Machine& machine) const;

	bool fitness_less_than(float a, float b) const { return a < b; }

	void clear_fitness_cache() { _fitness.clear(); }

	void initial_population(eugene::Random& rng,
	                        Population&     pop,
	                        size_t          gene_size,
	                        size_t          pop_size) const;

private:
	size_t distance(const std::vector<uint8_t>& source,
	                const std::vector<uint8_t>& target) const;

	// count
	/*struct FreqEvaluator : public Raul::MIDISink {
		Evaluator(const Problem& problem)
		: _problem(problem), _n_notes(0), _length(0) {
			for (uint8_t i=0; i < 128; ++i)
				_note_frequency[i] = 0;
		}
		void write_event(Raul::BeatTime time,
		                 size_t         ev_size,
		                 const uint8_t* ev) throw (std::logic_error);
		void compute();
		const Problem& _problem;

		size_t n_notes() const { return _n_notes; }

		float  _note_frequency[128];
		size_t _n_notes;
		double _length;
	};*/

	// distance
	/*struct Evaluator : public Raul::MIDISink {
		Evaluator(const Problem& problem) : _problem(problem) {}
		void write_event(Raul::BeatTime time,
		                 size_t         ev_size,
		                 const uint8_t* ev) throw (std::logic_error);
		void compute();
		const Problem& _problem;

		size_t n_notes() const { return _notes.size(); }

		std::vector<uint8_t> _notes;
		float                _counts[128];
	};*/

	struct Evaluator : public MIDISink {
		explicit Evaluator(const Problem& problem)
			: _problem(problem)
			, _order(4)
			, _n_notes(0)
			, _first_note(0)
		{
			for (uint8_t i=0; i < 128; ++i)
				_counts[i] = 0;
		}
		void write_event(TimeStamp      time,
		                 size_t         ev_size,
		                 const uint8_t* ev) throw (std::logic_error);
		void compute();
		const Problem& _problem;

		size_t n_notes() const { return _n_notes; }
		uint8_t first_note() const { return _first_note; }

		const uint32_t _order;

		std::string _read;

		typedef std::map<std::string, uint32_t> Patterns;
		Patterns _patterns;
		uint32_t _counts[128];
		size_t   _n_notes;
		uint8_t  _first_note;
	};

	TimeUnit           _unit;

	Evaluator     _target;
	SPtr<Machine> _seed;

	/// for levenshtein distance
	mutable std::vector< std::vector<uint16_t> > _matrix;

	/// memoization
	mutable std::map<Machine*, float> _fitness;
};

} // namespace machina

#endif // MACHINA_PROBLEM_HPP
