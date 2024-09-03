

roles:
- teacher: waits for all students to arrive first
           assigns group IDs to students, 
           and signal students when they can leave 
- student: arrives, waits to be assigned group IDs, then leave before teacher 

synchro requirements:

- teacher has to wait for all students to arrive
- student has to wait to be assigned a group before proceeding
- teacher has to signal when the students can leave (after all students have their groups)


