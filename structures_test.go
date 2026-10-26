package main

import (
	"math/rand"
	"reflect"
	"testing"
)

func TestListInvariants(t *testing.T) {
	for _, doubly := range []bool{false, true} {
		l := List{doubly: doubly}
		model := []int{}
		r := rand.New(rand.NewSource(2026))
		for step := 0; step < 2000; step++ {
			if len(model) == 0 || r.Intn(2) == 0 {
				pos := r.Intn(len(model) + 1)
				v := r.Intn(100)
				if e := l.Insert(pos, v); e != nil {
					t.Fatal(e)
				}
				model = append(model, 0)
				copy(model[pos+1:], model[pos:len(model)-1])
				model[pos] = v
			} else {
				pos := r.Intn(len(model))
				v, e := l.Delete(pos)
				if e != nil || v != model[pos] {
					t.Fatalf("delete: %v %v", v, e)
				}
				model = append(model[:pos], model[pos+1:]...)
			}
			if !reflect.DeepEqual(l.Values(false), model) {
				t.Fatalf("step %d: list differs", step)
			}
			var prev *Link
			for p := l.head; p != nil; p = p.next {
				if doubly && p.prev != prev {
					t.Fatal("broken backward link")
				}
				if !doubly && p.prev != nil {
					t.Fatal("singly linked node has prev")
				}
				prev = p
			}
			if prev != l.tail {
				t.Fatal("invalid tail")
			}
			rev := l.Values(true)
			for i, v := range rev {
				if v != model[len(model)-1-i] {
					t.Fatal("reverse traversal")
				}
			}
		}
	}
}
func TestArrayAgainstSlice(t *testing.T) {
	a := Array{}
	model := []int{}
	r := rand.New(rand.NewSource(42))
	for step := 0; step < 2000; step++ {
		if len(model) == 0 || r.Intn(2) == 0 {
			i := r.Intn(len(model) + 1)
			v := r.Intn(100)
			if e := a.Insert(i, v); e != nil {
				t.Fatal(e)
			}
			model = append(model, 0)
			copy(model[i+1:], model[i:len(model)-1])
			model[i] = v
		} else {
			i := r.Intn(len(model))
			v, e := a.Delete(i)
			if e != nil || v != model[i] {
				t.Fatal("delete")
			}
			model = append(model[:i], model[i+1:]...)
		}
		if a.size != len(model) {
			t.Fatal("length")
		}
		for i, v := range model {
			got, e := a.Get(i)
			if e != nil || got != v {
				t.Fatal("value")
			}
		}
	}
}
