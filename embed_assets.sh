#!/bin/sh -eu

out_file="src/assets_data.gen.h"

echo "#define NFILES $#" > "${out_file}.part1"
printf '#define ADD_ASSETS() \\\n' > "${out_file}.part2"
for asset ; do
	name="$(echo "${asset}" | tr '/\-\. ' '_')"
	printf 'vfs_add_file("%s", (void *)&%s[0], sizeof(%s)); \\\n' "${asset}" "${name}" "${name}" >> "${out_file}.part2"
	printf '#include "%s.h"\n' "../${asset}" >> "${out_file}.part1"
done
echo >> "${out_file}.part1"
echo ';' >> "${out_file}.part2"

printf '/*\n * generated on %s\n */\n' "$(date)" > "${out_file}"
cat "${out_file}.part1" "${out_file}.part2" >> "${out_file}"
rm "${out_file}.part1" "${out_file}.part2"
