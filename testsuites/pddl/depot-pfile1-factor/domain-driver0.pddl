(define (domain depot)
	(:requirements :factored-privacy :typing)
(:types
	 place locatable driver - object 
 	 depot distributor - place 
 	 truck hoist surface - locatable 
 	 pallet crate - surface 
 )
(:predicates
	(at ?x - locatable ?y - place)
	(on ?x - crate ?y - surface)
	(in ?x - crate ?y - truck)
	(clear ?x - surface)

	(:private
		(driving ?agent - driver ?t - truck)
	)
)

(:action drive
	:parameters (?a - driver ?x - truck ?y - place ?z - place)
	:precondition (and
		(at ?x ?y)
		(driving ?a ?x)
	)
	:effect (and
		(at ?x ?z)
		(not (at ?x ?y))
	)
)

)