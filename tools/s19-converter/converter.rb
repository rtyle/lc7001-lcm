def generate_s19(orig, mmap)
	puts orig.first
	
	(0..mmap.max.first).step(0x48) do |a|
		line = ""
		line += "S34D"
		line += a.to_s(16).rjust(8,"0")
		a.upto(a+0x48-1) do |k|
			line += mmap[k]?mmap[k]:"FF"
		end
		
		line += line[2..-1].scan(/.{2}/).map{|s| s.to_i(16)}.inject(:+).to_s(2).rjust(8,"0")[-8..-1].gsub(/./){|bit| bit=="1"?"0":"1"}.to_i(2).to_s(16).rjust(2,"0").upcase
		puts line
	end
	
	puts orig.last
end

def store_bytes(mmap, line)
	len = line.chop[2..3]
	start_addr = line.chop[4..11].to_i(16)
	data = line.chop[12..-3].scan(/.{2}/)
	addrs = start_addr.upto(start_addr+data.length-1).to_a
	mmap.merge!(Hash[addrs.zip(data)])	
end

def verify_length(l)
	(l.chop.length / 2) - 2 == l[2..3].to_i(16)
end

def verify_cs(l)
	l.chop[2..-3].scan(/.{2}/).map{|s| s.to_i(16)}.inject(:+).to_s(2).rjust(8,"0")[-8..-1].gsub(/./){|bit| bit=="1"?"0":"1"}.to_i(2).to_s(16).rjust(2,"0").upcase == l.chop[-2..-1]
end

fn = ARGV[0] ? ARGV[0] : 'stable.S19'
lines = IO.readlines(fn)
mmap = {}

lines.each_with_index do |l, i|	
	if l[0..1] != 'S3'
		#puts "INFO: Skipping non S3 line..."
		next
	end
	if not verify_length(l)
		puts "ERROR: Invalid length line! Aborting..."
		puts l
		abort
	end
	if not verify_cs(l)
		puts "ERROR: Invalid check sum line! Aborting..."
		puts l
		abort
	end
	
	store_bytes(mmap, l)			
end

generate_s19(lines, mmap)
