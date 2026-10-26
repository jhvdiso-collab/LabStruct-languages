package main

import (
	"bufio"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"sort"
	"strconv"
	"strings"
)

type Container struct {
	kind  byte
	array Array
	list  List
	stack Stack
	queue Queue
	tree  []int // C++ owns AVL operations; retain its records unchanged.
}
type Database map[string]*Container

func newContainer(kind byte) (*Container, error) {
	if !strings.ContainsRune("MFLSQT", rune(kind)) {
		return nil, fmt.Errorf("unknown type")
	}
	return &Container{kind: kind, list: List{doubly: kind == 'L'}}, nil
}
func (c *Container) Values(reverse bool) []int {
	switch c.kind {
	case 'M':
		return c.array.data[:c.array.size]
	case 'F', 'L':
		return c.list.Values(reverse)
	case 'S':
		out := []int{}
		for p := c.stack.top; p != nil; p = p.next {
			out = append(out, p.value)
		}
		return out
	case 'Q':
		out := []int{}
		for p := c.queue.head; p != nil; p = p.next {
			out = append(out, p.value)
		}
		return out
	default:
		return c.tree
	}
}
func number(s string) (int, error) {
	// Keep exactly the same signed 32-bit input grammar as the C++ CLI.
	if s == "" || s[0] == '+' {
		return 0, fmt.Errorf("expected a 32-bit integer")
	}
	v, e := strconv.ParseInt(s, 10, 32)
	return int(v), e
}
func validName(s string) bool {
	if s == "" {
		return false
	}
	for _, c := range s {
		if !(c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c >= '0' && c <= '9' || c == '_' || c == '-') {
			return false
		}
	}
	return true
}
func load(path string) (Database, error) {
	db := Database{}
	f, e := os.Open(path)
	if os.IsNotExist(e) {
		return db, nil
	}
	if e != nil {
		return nil, e
	}
	defer f.Close()
	r := bufio.NewReader(f)
	header, e := r.ReadString('\n')
	if e != nil || header != "LABSTRUCT 1\n" {
		return nil, fmt.Errorf("invalid file header")
	}
	for {
		line, e := r.ReadString('\n')
		if e != nil && e != io.EOF {
			return nil, e
		}
		if line == "" && e == io.EOF {
			break
		}
		row := strings.Fields(line)
		if len(row) < 3 || len(row[0]) != 1 || !validName(row[1]) || db[row[1]] != nil {
			return nil, fmt.Errorf("invalid container record")
		}
		count, err := number(row[2])
		if err != nil || count < 0 || count != len(row)-3 {
			return nil, fmt.Errorf("invalid element count")
		}
		c, err := newContainer(row[0][0])
		if err != nil {
			return nil, err
		}
		values := make([]int, count)
		for i, s := range row[3:] {
			values[i], err = number(s)
			if err != nil {
				return nil, err
			}
		}
		switch c.kind {
		case 'M':
			for _, v := range values {
				c.array.Push(v)
			}
		case 'F', 'L':
			// Loading uses a tail cursor, so even a singly linked list loads in O(n).
			for _, v := range values {
				node := &Link{value: v}
				if c.list.tail == nil {
					c.list.head = node
				} else {
					c.list.tail.next = node
					if c.list.doubly {
						node.prev = c.list.tail
					}
				}
				c.list.tail = node
			}
		case 'S':
			for i := len(values) - 1; i >= 0; i-- {
				c.stack.Push(values[i])
			}
		case 'Q':
			for _, v := range values {
				c.queue.Push(v)
			}
		case 'T':
			seen := map[int]bool{}
			for _, v := range values {
				if seen[v] {
					return nil, fmt.Errorf("duplicate AVL key")
				}
				seen[v] = true
			}
			c.tree = values
		}
		db[row[1]] = c
		if e == io.EOF {
			break
		}
	}
	return db, nil
}
func (db Database) Save(path string) (err error) {
	f, err := os.CreateTemp(filepath.Dir(path), filepath.Base(path)+".tmp.*")
	if err != nil {
		return err
	}
	defer func() { f.Close(); os.Remove(f.Name()) }()
	w := bufio.NewWriter(f)
	if _, err = fmt.Fprintln(w, "LABSTRUCT 1"); err != nil {
		return err
	}
	names := []string{}
	for name := range db {
		names = append(names, name)
	}
	sort.Strings(names)
	for _, name := range names {
		c := db[name]
		values := c.Values(false)
		if _, err = fmt.Fprintf(w, "%c %s %d", c.kind, name, len(values)); err != nil {
			return err
		}
		for _, v := range values {
			if _, err = fmt.Fprintf(w, " %d", v); err != nil {
				return err
			}
		}
		if _, err = fmt.Fprintln(w); err != nil {
			return err
		}
	}
	if err = w.Flush(); err != nil {
		return err
	}
	if err = f.Close(); err != nil {
		return err
	}
	return os.Rename(f.Name(), path)
}
func commands() map[string]int {
	m := map[string]int{"PRINT": 2, "MPUSH": 3, "MINSERT": 4, "MGET": 3, "MDEL": 3, "MSET": 4, "MLEN": 2, "SPUSH": 3, "SPOP": 2, "SGET": 2, "QPUSH": 3, "QPOP": 2, "QGET": 2}
	for _, c := range "MFLSQ" {
		m[string(c)+"CREATE"] = 2
	}
	for _, c := range "FL" {
		for _, op := range []string{"PUSH", "PUSHFRONT", "PUSHBACK", "DEL", "DELVAL", "GET", "FIND", "DELBEFORE", "DELAFTER"} {
			m[string(c)+op] = 3
		}
		for _, op := range []string{"PUSHBEFORE", "PUSHAFTER"} {
			m[string(c)+op] = 4
		}
		for _, op := range []string{"DELFRONT", "DELBACK", "LEN", "REVERSE"} {
			m[string(c)+op] = 2
		}
	}
	return m
}
func result(v int, changed bool, e error) (string, bool, error) { return strconv.Itoa(v), changed, e }
func (db Database) Execute(query string) (string, bool, error) {
	args := strings.Fields(query)
	if len(args) == 0 {
		return "", false, fmt.Errorf("empty query")
	}
	op := args[0]
	n, ok := commands()[op]
	if !ok {
		return "", false, fmt.Errorf("unknown command (AVL operations are available in C++)")
	}
	if len(args) != n || !validName(args[1]) {
		return "", false, fmt.Errorf("invalid arguments")
	}
	a, b := 0, 0
	var e error
	if n > 2 {
		a, e = number(args[2])
		if e != nil {
			return "", false, e
		}
	}
	if n > 3 {
		b, e = number(args[3])
		if e != nil {
			return "", false, e
		}
	}
	c := db[args[1]]
	if op == "PRINT" {
		if c == nil {
			return "", false, fmt.Errorf("container not found")
		}
		if c.kind == 'T' {
			return "", false, fmt.Errorf("use C++ for AVL operations")
		}
		return fmt.Sprint(c.Values(false)), false, nil
	}
	kind := op[0]
	action := op[1:]
	if c != nil && c.kind != kind {
		return "", false, fmt.Errorf("container type mismatch")
	}
	if action == "CREATE" && c != nil {
		return "", false, fmt.Errorf("container already exists")
	}
	if c == nil {
		if action != "CREATE" && action != "PUSH" && action != "PUSHFRONT" && action != "PUSHBACK" {
			return "", false, fmt.Errorf("container not found; use CREATE or PUSH")
		}
		c, e = newContainer(kind)
		if e != nil {
			return "", false, e
		}
		db[args[1]] = c
	}
	if action == "CREATE" {
		return "OK", true, nil
	}
	switch kind {
	case 'M':
		switch action {
		case "PUSH":
			c.array.Push(a)
			return result(a, true, nil)
		case "INSERT":
			return result(b, true, c.array.Insert(a, b))
		case "GET":
			v, e := c.array.Get(a)
			return result(v, false, e)
		case "DEL":
			v, e := c.array.Delete(a)
			return result(v, true, e)
		case "SET":
			return result(b, true, c.array.Set(a, b))
		case "LEN":
			return result(c.array.size, false, nil)
		}
	case 'F', 'L':
		switch action {
		case "PUSH", "PUSHBACK":
			c.list.PushBack(a)
			return result(a, true, nil)
		case "PUSHFRONT":
			return result(a, true, c.list.Insert(0, a))
		case "PUSHBEFORE", "PUSHAFTER":
			if _, e := c.list.At(a); e != nil {
				return "", false, e
			}
			if action == "PUSHAFTER" {
				a++
			}
			return result(b, true, c.list.Insert(a, b))
		case "GET":
			p, e := c.list.At(a)
			if e != nil {
				return "", false, e
			}
			return result(p.value, false, nil)
		case "FIND":
			return result(c.list.Find(a), false, nil)
		case "LEN":
			return result(c.list.Len(), false, nil)
		case "REVERSE":
			return fmt.Sprint(c.list.Values(true)), false, nil
		case "DELVAL":
			i := c.list.Find(a)
			if i < 0 {
				return "FALSE", false, nil
			}
			_, e := c.list.Delete(i)
			return "TRUE", true, e
		case "DELFRONT":
			a = 0
		case "DELBACK":
			v, e := c.list.PopBack()
			return result(v, true, e)
		case "DELBEFORE", "DELAFTER":
			if _, e := c.list.At(a); e != nil {
				return "", false, e
			}
			if action == "DELAFTER" {
				a++
			} else {
				a--
			}
		}
		v, e := c.list.Delete(a)
		return result(v, true, e)
	case 'S':
		if action == "PUSH" {
			c.stack.Push(a)
			return result(a, true, nil)
		}
		if action == "POP" {
			v, e := c.stack.Pop()
			return result(v, true, e)
		}
		v, e := c.stack.Peek()
		return result(v, false, e)
	case 'Q':
		if action == "PUSH" {
			c.queue.Push(a)
			return result(a, true, nil)
		}
		if action == "POP" {
			v, e := c.queue.Pop()
			return result(v, true, e)
		}
		v, e := c.queue.Peek()
		return result(v, false, e)
	}
	return "", false, fmt.Errorf("unknown command")
}
func run() error {
	if len(os.Args) == 2 && os.Args[1] == "--help" {
		fmt.Println("Usage: labstruct-go --file PATH --query 'COMMAND name [arguments]'\nTypes: M F L S Q. AVL operations use the C++ executable. See README.md.")
		return nil
	}
	file, query := "", ""
	for i := 1; i < len(os.Args); i += 2 {
		if i+1 >= len(os.Args) {
			return fmt.Errorf("missing option value")
		}
		if os.Args[i] == "--file" && file == "" {
			file = os.Args[i+1]
		} else if os.Args[i] == "--query" && query == "" {
			query = os.Args[i+1]
		} else {
			return fmt.Errorf("unknown or duplicate option")
		}
	}
	if file == "" || query == "" {
		return fmt.Errorf("required: --file PATH --query 'COMMAND name [arguments]'")
	}
	db, e := load(file)
	if e != nil {
		return e
	}
	out, changed, e := db.Execute(query)
	if e != nil {
		return e
	}
	if changed {
		if e = db.Save(file); e != nil {
			return e
		}
	}
	fmt.Println(out)
	return nil
}
func main() {
	if e := run(); e != nil {
		fmt.Fprintln(os.Stderr, "Error:", e)
		os.Exit(1)
	}
}
