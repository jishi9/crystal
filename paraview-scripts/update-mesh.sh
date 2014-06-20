PARTFILE=meshes/newnaca.p.part
MESHFILE=meshes/newnaca.p

#DETECT STRUCTURE
./detect_multiple_structure.py --random_seed 4 --max_regions 500 --max_fail=2000 "$PARTFILE" "$MESHFILE"

#Get region number AND size
bin/mesh-printer structure "$MESHFILE" | tail -n +5 | cut -d' ' -f 4,11 | tr -d ':' > /tmp/region



#GET OLDNEW node renumbering, ordered by new
unzip -p "$MESHFILE"  oldnode_to_newnode | sort -n -k2 > /tmp/oldnew