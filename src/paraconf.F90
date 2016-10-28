!/*******************************************************************************
! * Copyright (c) 2015, Julien Bigot | Mohamed Gaalich - CEA (julien.bigot@cea.fr)
! * All rights reserved.
! *
! * Redistribution and use in source and binary forms, with or without
! * modification, are permitted provided that the following conditions are met:
! * * Redistributions of source code must retain the above copyright
! *   notice, this list of conditions and the following disclaimer.
! * * Redistributions in binary form must reproduce the above copyright
! *   notice, this list of conditions and the following disclaimer in the
! *   documentation and/or other materials provided with the distribution.
! * * Neither the name of CEA nor the names of its contributors may be used to
! *   endorse or promote products derived from this software without specific 
! *   prior written permission.
! *
! * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
! * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
! * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
! * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
! * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
! * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
! * THE SOFTWARE.
! ******************************************************************************/
module paraconf_types

    USE iso_C_binding
 
    IMPLICIT NONE

    TYPE, bind(C) :: PC_tree_t
        INTEGER(C_INT) :: status
        TYPE(C_PTR) :: document
        TYPE(C_PTR) :: node
    END TYPE PC_tree_t

    TYPE, bind(C) :: PC_errhandler_t
        TYPE(C_FUNPTR) :: func
        TYPE(C_PTR) :: context
    END TYPE PC_errhandler_t

endmodule
MODULE paraconf

    USE iso_C_binding
    USE paraconf_types

    IMPLICIT NONE

    !! Status of function execution
    ENUM, BIND(C)
        ENUMERATOR :: PC_OK = 0            ! No error
        ENUMERATOR :: PC_INVALID_PARAMETER ! A parameter value is invalid
        ENUMERATOR :: PC_INVALID_NODE_TYPE ! Unexpected type found for a node
        ENUMERATOR :: PC_NODE_NOT_FOUND    ! The requested node doesn exist in the tree
        ENUMERATOR :: PC_INVALID_FORMAT    ! The provided input is invalid
        ENUMERATOR :: PC_SYSTEM_ERROR      ! An error occured with the system
    END ENUM

    ! Error handlers
    TYPE(PC_errhandler_t), BIND(C, NAME="PC_ASSERT_HANDLER") :: PC_ASSERT_HANDLER
    TYPE(PC_errhandler_t), BIND(C, NAME="PC_NULL_HANDLER") :: PC_NULL_HANDLER

    ! character length parameters 
    INTEGER, PARAMETER :: PC_ERRMSG_MAXLENGTH = 255

    INTERFACE
        TYPE(C_PTR) &
          FUNCTION PC_errmsg_f() &
            bind(C, name="PC_errmsg")
            USE ISO_C_BINDING
          END FUNCTION PC_errmsg_f
    END INTERFACE

    INTERFACE
        TYPE(PC_errhandler_t) &
          FUNCTION PC_errhandler_f(handler) &
            bind(C, name="PC_errhandler")
            USE iso_C_binding
            USE paraconf_types
            TYPE(PC_errhandler_t), VALUE :: handler
          END FUNCTION PC_errhandler_f
    END INTERFACE

    INTERFACE
        TYPE(PC_tree_t) & 
          FUNCTION PC_parse_path_f(path) &
            bind(C, name="PC_parse_path")   
            USE iso_C_binding 
            USE paraconf_types
            TYPE(C_PTR), VALUE :: path
          END FUNCTION PC_parse_path_f
    END INTERFACE

    INTERFACE
        TYPE(PC_tree_t) &
          FUNCTION PC_get_f0_c(tree,index_fmt) &
            bind(C, name="PC_get")   
            USE iso_C_binding 
            USE paraconf_types
            TYPE(PC_tree_t), VALUE :: tree
            TYPE(C_PTR), VALUE :: index_fmt
          END FUNCTION PC_get_f0_c
    END INTERFACE

    ! PC_get functions Generic interface to emulate variable argument list
    INTERFACE PC_get
        MODULE PROCEDURE PC_get_f0
    END INTERFACE PC_get

    INTERFACE
        INTEGER(C_INT) &
          FUNCTION PC_len_f(tree,value) &
            bind(C, name="PC_len")   
            USE iso_C_binding 
            USE paraconf_types
            TYPE(PC_tree_t), VALUE :: tree
            TYPE(C_PTR), VALUE :: value
          END FUNCTION PC_len_f
    END INTERFACE

    INTERFACE
        INTEGER(C_INT) &
          FUNCTION PC_int_f(tree,value) &
            bind(C, name="PC_int")   
            USE iso_C_binding 
            USE paraconf_types
            TYPE(PC_tree_t), VALUE :: tree
            TYPE(C_PTR), VALUE :: value
          END FUNCTION PC_int_f
    END INTERFACE

    INTERFACE
        INTEGER(C_INT) &
          FUNCTION PC_double_f(tree,value) &
            bind(C, name="PC_double")   
            USE iso_C_binding 
            USE paraconf_types
            TYPE(PC_tree_t), VALUE :: tree
            TYPE(C_PTR), VALUE :: value
          END FUNCTION PC_double_f
    END INTERFACE

    INTERFACE
        INTEGER(C_INT) &
          FUNCTION PC_string_f(tree,value) &
            bind(C, name="PC_string")   
            USE iso_C_binding 
            USE paraconf_types
            TYPE(PC_tree_t), VALUE :: tree
            TYPE(C_PTR), VALUE :: value
          END FUNCTION PC_string_f
    END INTERFACE


    INTERFACE
        INTEGER(C_INT) &
          FUNCTION PC_tree_destroy_f(tree) &
            bind(C, name="PC_tree_destroy")   
            USE iso_C_binding 
            USE paraconf_types
            TYPE(PC_tree_t) :: tree
          END FUNCTION PC_tree_destroy_f
    END INTERFACE

    INTERFACE
        SUBROUTINE free_f(ptr) &
            bind(C, name="free")   
            USE iso_C_binding 
            TYPE(C_PTR), VALUE :: ptr
        END SUBROUTINE free_f
    END INTERFACE

    CONTAINS 

    !==================================================================
    SUBROUTINE PC_status(tree,status)
        TYPE(PC_tree_t), INTENT(IN) :: tree
        INTEGER, INTENT(OUT) :: status

        status = int(tree%status,kind(status))

    END SUBROUTINE PC_status
    !==================================================================

   

    !==================================================================
    SUBROUTINE PC_errmsg(errmsg)
        CHARACTER(*), INTENT(OUT) :: errmsg
        CHARACTER(KIND=C_CHAR), POINTER, DIMENSION(:) :: errmsg_array
        CHARACTER(LEN=:), ALLOCATABLE :: tmpmsg
        INTEGER :: errmsg_length
        INTEGER :: I

        errmsg_length = LEN(errmsg)
        errmsg = ""
        CALL C_F_POINTER(PC_errmsg_f(), errmsg_array, [errmsg_length])
        IF (ASSOCIATED(errmsg_array)) THEN

           ! The allocate statement does not work with gfortran < 4.8 
           ! so we use the suggested work around
           ! https://gcc.gnu.org/bugzilla/show_bug.cgi?id=51055
           ! ALLOCATE(CHARACTER(errmsg_length)::tmpmsg)     
           tmpmsg = (REPEAT(' ', errmsg_length))

            DO I = 1, errmsg_length
                tmpmsg(i:i+1) = errmsg_array(i)
                IF (errmsg_array(i) == CHAR(0)) EXIT                
            END DO

            errmsg = tmpmsg(1:I-1)

            ! rely on automatic deallocation?
            ! IF (ALLOCATED(tmpmsg)) DEALLOCATE(tmpmsg)
        END IF

    END SUBROUTINE PC_errmsg
    !==================================================================    

    !==================================================================
    SUBROUTINE PC_errhandler(new_handler, old_handler)
        TYPE(PC_errhandler_t), INTENT(IN) :: new_handler
        TYPE(PC_errhandler_t), INTENT(OUT), OPTIONAL :: old_handler

        TYPE(PC_errhandler_t) :: tmp_handler

        IF (PRESENT(old_handler)) THEN
            old_handler = PC_errhandler_f(new_handler)
        ELSE
            tmp_handler = PC_errhandler_f(new_handler)
        END IF

    END SUBROUTINE PC_errhandler
    !==================================================================


    !==================================================================
    SUBROUTINE PC_parse_path(path,tree)
        CHARACTER(LEN=*), INTENT(IN) :: path
        TYPE(PC_tree_t), INTENT(OUT) :: tree

        INTEGER :: i
        CHARACTER(C_CHAR), TARGET :: C_path(len_trim(path)+1)

        do i=1,len_trim(path)
              C_path(i) = path(i:i)
        end do
        C_path(len_trim(path)+1) = C_NULL_CHAR

        tree = PC_parse_path_f(c_loc(C_path))

    END SUBROUTINE PC_parse_path
    !==================================================================
    
    
    
    !==================================================================
    SUBROUTINE PC_len(tree_in,value,status)
        TYPE(PC_tree_t), INTENT(IN) :: tree_in
        INTEGER, INTENT(OUT), TARGET :: value
        INTEGER, INTENT(OUT), OPTIONAL :: status

        INTEGER :: tmp
        

        if(PRESENT(status)) then
            status = int(PC_len_f(tree_in,c_loc(value)))
        else
            tmp = int(PC_len_f(tree_in,c_loc(value)))
        end if

    END SUBROUTINE PC_len
    !==================================================================
    
    !==================================================================
    TYPE(PC_tree_t) FUNCTION PC_get_f0(tree, index_fmt)
        TYPE(PC_tree_t), INTENT(IN) :: tree
        CHARACTER(LEN=*), INTENT(IN) :: index_fmt

        INTEGER :: i
        CHARACTER(C_CHAR), TARGET :: C_index_fmt(len_trim(index_fmt)+1)

        do i=1,len_trim(index_fmt)
              C_index_fmt(i) = index_fmt(i:i)
        end do
        C_index_fmt(len_trim(index_fmt)+1) = C_NULL_CHAR

        PC_get_f0 = PC_get_f0_c(tree,c_loc(C_index_fmt))

    END FUNCTION PC_get_f0
    !==================================================================
    
    !==================================================================
    SUBROUTINE PC_int(tree_in,value,status)
        TYPE(PC_tree_t), INTENT(IN) :: tree_in
        INTEGER, INTENT(OUT), TARGET :: value
        INTEGER, INTENT(OUT), OPTIONAL :: status

        INTEGER :: tmp

        if(PRESENT(status)) then
            status = int(PC_int_f(tree_in,c_loc(value)))
        else
            tmp = int(PC_int_f(tree_in,c_loc(value)))
        end if

        !value = Fp_value
        !print *, "value =",Fp_value

    END SUBROUTINE PC_int
    !==================================================================
    
    
    
    !==================================================================
    SUBROUTINE PC_double(tree_in,value,status)
        TYPE(PC_tree_t), INTENT(IN) :: tree_in
        REAL(8), INTENT(OUT), TARGET :: value
        INTEGER, INTENT(OUT), OPTIONAL :: status

        INTEGER :: tmp

        if(PRESENT(status)) then
            status = int(PC_double_f(tree_in,c_loc(value)))
        else
            tmp = int(PC_double_f(tree_in,c_loc(value)))
        end if

    END SUBROUTINE PC_double
    !==================================================================
    
    
    
    !==================================================================
    SUBROUTINE PC_string(tree_in,value,status)
        TYPE(PC_tree_t), INTENT(IN) :: tree_in
        CHARACTER(LEN=*), INTENT(OUT) :: value
        INTEGER, INTENT(OUT), OPTIONAL :: status

        INTEGER :: i,tmp
        INTEGER, DIMENSION(1) :: tab_lengh
        TYPE(C_PTR),TARGET :: C_pointer
        CHARACTER, DIMENSION(:), POINTER :: F_pointer


        if(PRESENT(status)) then
            status = int(PC_string_f(tree_in,c_loc(C_pointer)))
        else
            tmp = int(PC_string_f(tree_in,c_loc(C_pointer)))
        end if

        call PC_len(tree_in,tab_lengh(1))  

        call c_f_pointer(C_pointer,F_pointer,tab_lengh)

        do i=1,tab_lengh(1)
            value(i:i) = F_pointer(i)
        end do

        do i=tab_lengh(1)+1,len(value)
            value(i:i) = ' '
        end do 

        call free_f(C_pointer)

    END SUBROUTINE PC_string
    !==================================================================
    
    
    
    !==================================================================
    SUBROUTINE PC_tree_destroy(tree_in,status)
        TYPE(PC_tree_t), INTENT(INOUT), TARGET :: tree_in
        INTEGER, INTENT(OUT), OPTIONAL :: status

        INTEGER :: tmp

        if(PRESENT(status)) then
            status = int(PC_tree_destroy_f(tree_in))
        else
            tmp = int(PC_tree_destroy_f(tree_in))
        end if

    END SUBROUTINE PC_tree_destroy
    !==================================================================

END MODULE paraconf
!======================================================================
